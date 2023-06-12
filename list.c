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

//    List nextNode = list->next;
//    int retData = nextNode->data;
//    list->next = nextNode->next;
//    free(nextNode);
//    (*size)--;

    return removeNode(list, size);
}

void removeIndexes(List list, int* indexes_to_remove, int num_indexes, int* list_size, int* removed) {
    List currentNode = list;
    int index = 0;
    while(currentNode != NULL) {
        if(is_in_array(indexes_to_remove,num_indexes, index)) {
            removed[index] = removeNode(currentNode, list_size);
            index++;
            continue;
        }
        currentNode = currentNode->next;
        index++;
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
        if(array[0] == value)
            return 1;
    }
    return 0;
}

int removeNode(List list, int* list_size) {
    List nextNode = list->next;
    int retData = nextNode->data;
    list->next = nextNode->next;
    free(nextNode);
    (*list_size)--;

    return retData;
}