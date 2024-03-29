#include "segel.h"
#include "request.h"
#include "list.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


struct ThreadHandlerArgs_t {
    List list;
    int threadNum;
};

typedef struct ThreadHandlerArgs_t* ThreadHandlerArgs;

pthread_cond_t workerThreadCond;
pthread_cond_t mainThreadCond;
pthread_mutex_t m;
int listSize = 0;
int numWorkingThreads = 0;


void getargs(int *port, int *numThreads, int* queueSize, char** schedalg, int *maxSize, int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port> <num_threads> <queue_size> <schedalg> <?max_size>\n", argv[0]);
        exit(1);
    }

    *port = atoi(argv[1]);
    *numThreads = atoi(argv[2]);
    *queueSize = atoi(argv[3]);
    *schedalg = argv[4];

    if(strcmp(*schedalg, "block") != 0 &&
       strcmp(*schedalg, "dh") != 0 &&
       strcmp(*schedalg, "dt") != 0 &&
       strcmp(*schedalg, "bf") != 0 &&
       strcmp(*schedalg, "dynamic") != 0 &&
       strcmp(*schedalg, "random") != 0) {
        exit(1);
    }

    if(argc == 6) {
        *maxSize = atoi(argv[5]);
    }
}

void* threadHandler(void* args) {
    int connfd;
    ThreadHandlerArgs hargs = (ThreadHandlerArgs) args;
    List list = hargs->list;

    Stats stats = malloc(sizeof(Stats));
    ThreadStats tstats = malloc(sizeof(ThreadStats));

    tstats->tid = hargs->threadNum;


    tstats->reqCount = 0;
    tstats->staticReqCount = 0;
    tstats->dynamicReqCount = 0;

    while(1) {
        pthread_mutex_lock(&m);
        while(listSize == 0) {
            pthread_cond_wait(&workerThreadCond, &m);
        }

        ListItem item = removeFirst(list, &listSize);
        numWorkingThreads++;
        pthread_mutex_unlock(&m);

        struct timeval pickupTime;
        gettimeofday(&pickupTime, NULL);

        stats->arrivalTime = item->arrivalTime;
        connfd = item->connFd;

        timersub(&pickupTime, &stats->arrivalTime, &stats->dispatchInterval);
        tstats->reqCount++;
        requestHandle(item->connFd, stats, tstats);
        Close(connfd);

        pthread_mutex_lock(&m);
        numWorkingThreads--;
        pthread_cond_signal(&mainThreadCond);
        pthread_mutex_unlock(&m);
    }

}

void addRequest(List list, struct timeval arrivalTime, int connfd) {
    addNode(list, connfd, arrivalTime, &listSize);
    pthread_cond_signal(&workerThreadCond);
}

void handleBlock(List list, struct timeval arrivalTime, int connfd, int queueSize) {
    while(listSize+numWorkingThreads == queueSize) {
        pthread_cond_wait(&mainThreadCond, &m);
    }

    addRequest(list, arrivalTime ,connfd);
}

void handleDropTail(int connfd) {
    Close(connfd);
}

void handleDropHead(List list, struct timeval arrivalTime, int connfd) {
    if(listSize == 0) {
        addRequest(list, arrivalTime, connfd);
        return;
    }

    ListItem removedItem = removeFirst(list, &listSize);

    handleDropTail(removedItem->connFd);
    addRequest(list, arrivalTime, connfd);
}

void handleBlockFlush(int connfd) {
    while((listSize == 0) && (numWorkingThreads == 0)) {
        pthread_cond_wait(&mainThreadCond, &m);
    }
    Close(connfd);
}



void handleDynamic(int* queueSize, int connfd, int maxSize) {
    if(*queueSize < maxSize) {
        (*queueSize)++;
    }

    Close(connfd);
}

void randomizeIndexes(int* index_arr, int arr_size) {
    int res = 0;
    int* index_hist = malloc(sizeof(int) * arr_size);

    for(int i = 0; i < arr_size; i++) {
        index_hist[i] = 0;
    }

    srand(time(NULL));

    for(int i = 0; i < arr_size; i++)
    {
        do {
            res = rand() % listSize;
        }
        while(index_hist[res] != 0);

        index_arr[i] = res;
        index_hist[res]++;
    }

    free(index_hist);
}

void handleRandom(List list, struct timeval arrivalTime, int connfd) {
    if(listSize == 0) {
        addRequest(list, arrivalTime, connfd);
        return;
    }
    int num_of_indexes = 0;
    if (listSize % 2 == 0)
        num_of_indexes = listSize / 2;
    else
        num_of_indexes = (listSize + 1) / 2;

    int* indexes_to_remove = malloc(sizeof(int) * num_of_indexes);
    int* removed_requests = malloc(sizeof(int) * num_of_indexes);
    randomizeIndexes(indexes_to_remove, num_of_indexes);
    removeIndexes(list, indexes_to_remove, num_of_indexes, &listSize, removed_requests);

    for(int i = 0; i < num_of_indexes; i++) {

        Close(removed_requests[i]);
    }

    addRequest(list, arrivalTime, connfd);
    free(indexes_to_remove);
    free(removed_requests);
}

void handleSchedAlg(List list, char* schedalg, int connfd, int* queueSize, int maxSize, struct timeval arrivalTime) {
    if(strcmp(schedalg, "block") == 0) {
        handleBlock(list, arrivalTime, connfd, *queueSize);
        return;
    }

    if(strcmp(schedalg, "dt") == 0) {
        handleDropTail(connfd);
        return;
    }

    if(strcmp(schedalg, "dh") == 0) {
        handleDropHead(list, arrivalTime, connfd);
        return;
    }

    if(strcmp(schedalg, "bf") == 0) {
        handleBlockFlush(connfd);
        return;
    }

    // TODO: make sure that Dynamic is with capital D
    if(strcmp(schedalg, "dynamic") == 0) {
        handleDynamic(queueSize, connfd, maxSize);
        return;
    }

    if(strcmp(schedalg, "random") == 0) {
        handleRandom(list, arrivalTime, connfd);
        return;
    }
}

int main(int argc, char *argv[])
{
    pthread_cond_init(&workerThreadCond, NULL);
    pthread_cond_init(&mainThreadCond, NULL);
    pthread_mutex_init(&m, NULL);
    int listenfd, connfd, port, numThreads, queueSize, maxSize, clientlen;
    char* schedalg;
    struct sockaddr_in clientaddr;
    getargs(&port, &numThreads, &queueSize, &schedalg, &maxSize, argc, argv);

    List waitingList = init();

    pthread_t* workingThreads = malloc((sizeof(pthread_t) * numThreads));

    for(int i=0; i<numThreads; i++) {
        ThreadHandlerArgs args = malloc(sizeof(struct ThreadHandlerArgs_t));
        args->list = waitingList;
        args->threadNum = i;
        pthread_create(&workingThreads[i], NULL, threadHandler, args);
    }

    listenfd = Open_listenfd(port);

    while (1) {
        clientlen = sizeof(clientaddr);
        struct timeval arrivalTime;
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&arrivalTime, NULL);
        pthread_mutex_lock(&m);

        if(listSize+numWorkingThreads == queueSize) {
            handleSchedAlg(waitingList, schedalg, connfd, &queueSize, maxSize, arrivalTime);
        } else {
            addRequest(waitingList, arrivalTime, connfd);
        }

        pthread_mutex_unlock(&m);
    }

}


    


 
