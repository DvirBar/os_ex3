#include <stdlib.h>
#include "list.h"
#include "assert.h"
#include <stdio.h>

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
            removed[removed_index] = removeNode(list, nodeToRemove, list_size);
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

int removeNode(List head, List nodeToRemove, int* list_size) {
    List currentNode = head->next;
    List lastNode = head;
    while(currentNode != nodeToRemove) {
        lastNode = currentNode;
        currentNode = currentNode->next;
    }

    int retData = currentNode->data;
    lastNode->next = currentNode->next;
    free(currentNode);
    (*list_size)--;

    return retData;
}