#include <pthread.h>
#include <stdlib.h>
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

pthread_cond_t c = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int listSize = 0;
int numWorkingThreads = 0;


void getargs(int *port, int *numThreads, int* queueSize, char** schedalg, int *maxSize, int argc, char *argv[])
{
    if (argc < 5) {
        // TODO: what to print
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    *port = atoi(argv[1]);
    *numThreads = atoi(argv[2]);
    *queueSize = atoi(argv[3]);
    *schedalg = argv[4];

    if(argc == 6) {
        *maxSize = atoi(argv[5]);
    }
}

void* threadHandler(void* args) {
    int connfd = 0;
    List list = (List) args;
    while(1) {
        pthread_mutex_lock(&m);
        while(listSize == 0) {
            pthread_cond_wait(&c, &m);
        }

        connfd = removeFirst(list, &listSize);
        numWorkingThreads++;
        pthread_mutex_unlock(&m);

        requestHandle(connfd);
        Close(connfd);

        pthread_mutex_lock(&m);
        numWorkingThreads--;
        pthread_mutex_unlock(&m);
    }

}



int main(int argc, char *argv[])
{
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

    while (1) {
        clientlen = sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        pthread_mutex_lock(&m);
        if(listSize+numWorkingThreads == queueSize) {
            pthread_mutex_unlock(&m);
            continue;
        }

        addNode(waitingList, connfd, &listSize);
        pthread_cond_signal(&c);
        pthread_mutex_unlock(&m);
    }

}


    


 
