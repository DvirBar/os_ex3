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

//typedef struct ThreadHandlerArgs_t *ThreadHandlerArgs;
//struct ThreadHandlerArgs_t {
//    List workersQueue;
//    List holdingQueue;
//};

pthread_cond_t c;
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
       strcmp(*schedalg, "drop_head") != 0 &&
       strcmp(*schedalg, "drop_tail") != 0 &&
       strcmp(*schedalg, "block_flush") != 0 &&
       strcmp(*schedalg, "Dynamic") != 0 &&
       strcmp(*schedalg, "drop_random") != 0) {
        fprintf(stderr, "Invalid schedalg! Aborting...\n");
        exit(1);
    }

    if(argc == 6) {
        *maxSize = atoi(argv[5]);
    }
}

void* threadHandler(void* args) {
    int connfd = 0;
    List list = (List) args;
    uint64_t tid;
//    pthread_threadid_np(NULL, &tid);
    while(1) {
        pthread_mutex_lock(&m);
        while(listSize == 0) {
//            printf("%llu waiting...\n", tid);
            pthread_cond_wait(&c, &m);
        }

//        printf("%llu starting job.\n", tid);
        connfd = removeFirst(list, &listSize);
        printf("executing %d\n", connfd);
        numWorkingThreads++;
        pthread_mutex_unlock(&m);

        requestHandle(connfd);
        sleep(10);
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
    printf("dropping %d\n", removedConnFd);
    Close(removedConnFd);
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

    printf("dropping %d. queue size is now: %d\n", connfd, *queueSize);
    // TODO: replace close socket by calling to drop tail
    Close(connfd);
}

void randomizeIndexes(int* index_arr, int arr_size) {
    int res = 0;
    int* index_hist = malloc(sizeof(int) * arr_size);

    for(int i = 0; i < arr_size; i++)
    {
        do {
            res = rand() % arr_size;
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
        Close(removed_requests[i]);
    }

    addRequest(list, connfd);
    free(indexes_to_remove);
}

void handleSchedAlg(List list, char* schedalg, int connfd, int* queueSize, int maxSize) {
    // TODO: Should we put the lock inside for performance?
    if(strcmp(schedalg, "block") == 0) {
        handleBlock(list, connfd, *queueSize);
        return;
    }

    if(strcmp(schedalg, "drop_tail") == 0) {
        handleDropTail(connfd);
        return;
    }

    if(strcmp(schedalg, "drop_head") == 0) {
        handleDropHead(list, connfd);
        return;
    }

    if(strcmp(schedalg, "block_flush") == 0) {
        handleBlockFlush(connfd);
        return;
    }

    // TODO: make sure that Dynamic is with capital D
    if(strcmp(schedalg, "Dynamic") == 0) {
        handleDynamic(queueSize, connfd, maxSize);
        return;
    }

    if(strcmp(schedalg, "drop_random") == 0) {
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

    for(int i=0; i<numThreads; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, threadHandler, waitingList);
    }

    listenfd = Open_listenfd(port);

    uint64_t tid;
//    pthread_threadid_np(NULL, &tid);
    while (1) {
        clientlen = sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        printf("received %d\n", connfd);
        pthread_mutex_lock(&m);

        if(listSize+numWorkingThreads == queueSize) {
            handleSchedAlg(waitingList, schedalg, connfd, &queueSize, maxSize);
        } else {
            addRequest(waitingList, connfd);
        }

        pthread_mutex_unlock(&m);
    }

}


    


 
