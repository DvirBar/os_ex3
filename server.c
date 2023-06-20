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
    Stats stats;
    int threadNum;
};

typedef struct ThreadHandlerArgs_t* ThreadHandlerArgs;

pthread_cond_t c;
pthread_mutex_t m;
int listSize = 0;
int numWorkingThreads = 0;
int totalThreads = 0;


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
//        fprintf(stderr, "Invalid schedalg! Aborting...\n");
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
    Stats stats = hargs->stats;
//    uint64_t tid;

    ThreadStats tstats = malloc(sizeof(ThreadStats));
    tstats->tid = totalThreads;

    totalThreads++;

    tstats->reqCount = 0;
    tstats->staticReqCount = 0;
    tstats->dynamicReqCount = 0;
//    pthread_threadid_np(NULL, &tid);

//    printf("%d\n", hargs->threadNum);
    // TODO: we might want to change that

    while(1) {
        pthread_mutex_lock(&m);
        while(listSize == 0) {
//            printf("%llu, number: %d, waiting...\n", tid, tstats->tid);
            pthread_cond_wait(&c, &m);
        }

//        printf("%llu starting job.\n", tid);
        connfd = removeFirst(list, &listSize);
//        printf("executing %d by %d\n", connfd, tstats->tid);
        numWorkingThreads++;
        pthread_mutex_unlock(&m);
        // TODO: should it include this one?
        tstats->reqCount++;
        struct timeval pickupTime;
        gettimeofday(&pickupTime, NULL);

        timersub(&pickupTime, &stats->arrivalTime, &stats->dispatchInterval);
        requestHandle(connfd, stats, tstats);
        Close(connfd);

        pthread_mutex_lock(&m);
        numWorkingThreads--;
        pthread_cond_signal(&c);
        pthread_mutex_unlock(&m);
    }

}

void addRequest(List list, int connfd) {
    addNode(list, connfd, &listSize);
    pthread_cond_signal(&c);
}

void handleBlock(List list, int connfd, int queueSize) {
    // TODO: we wake up both the main thread and other threads, is it working fine?
    while(listSize+numWorkingThreads == queueSize) {
        pthread_cond_wait(&c, &m);
    }

    addRequest(list, connfd);
}

void handleDropTail(int connfd) {
//    printf("dropping %d\n", connfd);
    Close(connfd);
}

void handleDropHead(List list, int connfd) {
    int removedConnFd = removeFirst(list, &listSize);
    handleDropTail(removedConnFd);
    addRequest(list, connfd);
}

void handleBlockFlush(int connfd) {
    while((listSize == 0) && (numWorkingThreads == 0)) {
        pthread_cond_wait(&c, &m);
    }
    Close(connfd);
}



void handleDynamic(int* queueSize, int connfd, int maxSize) {
    if(*queueSize < maxSize) {
        (*queueSize)++;
    }

//    printf("dropping %d. queue size is now: %d\n", connfd, *queueSize);
    // TODO: replace close socket by calling to drop tail
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

void handleRandom(List list, int connfd) {
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
//        printf("closing %d\n", removed_requests[i]);
        Close(removed_requests[i]);
    }

    addRequest(list, connfd);
    free(indexes_to_remove);
    free(removed_requests);
}

void handleSchedAlg(List list, char* schedalg, int connfd, int* queueSize, int maxSize) {
    // TODO: Should we put the lock inside for performance?
    if(strcmp(schedalg, "block") == 0) {
        handleBlock(list, connfd, *queueSize);
        return;
    }

    if(strcmp(schedalg, "dt") == 0) {
        handleDropTail(connfd);
        return;
    }

    if(strcmp(schedalg, "dh") == 0) {
        handleDropHead(list, connfd);
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
        handleRandom(list, connfd);
        return;
    }
}

int main(int argc, char *argv[])
{
    pthread_cond_init(&c, NULL);
    pthread_mutex_init(&m, NULL);
    int listenfd, connfd, port, numThreads, queueSize, maxSize, clientlen;
    char* schedalg;
    struct sockaddr_in clientaddr;
    getargs(&port, &numThreads, &queueSize, &schedalg, &maxSize, argc, argv);

    List waitingList = init();
    Stats stats = malloc(sizeof(Stats));

    for(int i=0; i<numThreads; i++) {
        ThreadHandlerArgs args = malloc(sizeof(ThreadHandlerArgs));
        args->list = waitingList;
        args->stats = stats;
        args->threadNum = i;
        pthread_t thread;
        pthread_create(&thread, NULL, threadHandler, args);
    }

    listenfd = Open_listenfd(port);

    uint64_t tid;
    pthread_threadid_np(NULL, &tid);

    while (1) {
        clientlen = sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        // TODO: should we check for gettimeofday failure?
        gettimeofday(&stats->arrivalTime, NULL);
//        printf("received %d\n", connfd);
        pthread_mutex_lock(&m);

        if(listSize+numWorkingThreads == queueSize) {
            handleSchedAlg(waitingList, schedalg, connfd, &queueSize, maxSize);
        } else {
            addRequest(waitingList, connfd);
        }

        pthread_mutex_unlock(&m);
    }

}


    


 
