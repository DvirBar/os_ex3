#include <stdlib.h>
#include "list.h"
#include "assert.h"

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
    assert(list != NULL);

    List newNode = malloc(sizeof(*newNode));
    if(newNode == NULL) {
        return NULL;
    }

    List currentNode = list;
    while(currentNode->next != NULL) {
        currentNode = currentNode->next;
    }
    newNode->data = data;
    currentNode->next = newNode;
    newNode->next = NULL;
    (*size)++;

    return newNode;
}

int removeFirst(List list, int* size) {
    assert(list != NULL);
    if(!(list->next)) {
        return -1;
    }

    List nextNode = list->next;
    int retData = nextNode->data;
    list->next = nextNode->next;
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