#include <stdlib.h>
#include "list.h"

struct List_t {
    int data;
    struct List_t* next;
};

List init() {
    List list = malloc(sizeof(*list));
    if(list == NULL) {
        return NULL;
    }

    list->data = -1;
    list->next = NULL;

    return list;
}

List addNode(List list, int data, int* size) {
    List newNode = malloc(sizeof(*newNode));
    if(newNode == NULL) {
        return NULL;
    }

    newNode->data = data;
    list->next = newNode;
    newNode->next = NULL;
    (*size)++;

    return newNode;
}

int removeFirst(List list, int* size) {
    if(!list->next) {
        return -1;
    }

    List nextNode = list->next;
    list->next = nextNode->next;
    int retData = list->data;
    free(nextNode);
    (*size)--;

    return retData;
}

void deleteList(List list) {
    List currentNode = list;
    List temp = NULL;
    while(currentNode != NULL) {
        temp = currentNode;
        currentNode = currentNode->next;
        free(temp);
    }
}