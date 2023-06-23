#include <stdlib.h>
#include "list.h"
#include "assert.h"
#include <stdio.h>



List init() {
    List list = malloc(sizeof(struct List_t));
    if(list == NULL) {
        return NULL;
    }

    ListItem newItem = malloc(sizeof (struct ListItem_t));
    newItem->connFd = -1;

    list->data = newItem;
    list->next = NULL;

    return list;
}

List addNode(List list, int connFd, struct timeval arrivalTime, int* size) {
    assert(list != NULL);

    List newNode = malloc(sizeof(struct List_t));
    if(newNode == NULL) {
        return NULL;
    }

    List currentNode = list;
    while(currentNode->next != NULL) {
        currentNode = currentNode->next;
    }

    ListItem newItem = malloc(sizeof(struct ListItem_t));
    newItem->connFd = connFd;
    newItem->arrivalTime = arrivalTime;

    newNode->data = newItem;
    newNode->next = NULL;

    currentNode->next = newNode;
    (*size)++;

    return newNode;
}

ListItem removeFirst(List list, int* size) {
    assert(list != NULL);
    if(!(list->next)) {
        return list->data;
    }

    return removeNode(list, list->next, size);
}

void removeIndexes(List list, int* indexes_to_remove, int num_indexes, int* list_size, int* removed) {
    List currentNode = list->next;
    int index = 0;
    int removed_index = 0;
    List nodeToRemove = NULL;
//    for(int i=0; i < num_indexes; i++) {
//        printf("index: %d\n", indexes_to_remove[i]);
//    }
    while(currentNode != NULL) {
        if(is_in_array(indexes_to_remove,num_indexes, index)) {
            nodeToRemove = currentNode;
            currentNode = currentNode->next;
            removed[removed_index] = removeNode(list, nodeToRemove, list_size)->connFd;
//            printf("removed: %d\n", removed[removed_index]);
            index++;
            removed_index++;
        } else {
            currentNode = currentNode->next;
            index++;
        }
    }
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

int is_in_array(int* array, int array_size, int value) {
    for(int i = 0; i < array_size; i++) {
        if(array[i] == value)
            return 1;
    }
    return 0;
}

ListItem removeNode(List head, List nodeToRemove, int* list_size) {
    List currentNode = head->next;
    List lastNode = head;
    while(currentNode != nodeToRemove) {
        lastNode = currentNode;
        currentNode = currentNode->next;
    }

    ListItem retData = currentNode->data;
    lastNode->next = currentNode->next;
    free(currentNode);
    (*list_size)--;

    return retData;
}