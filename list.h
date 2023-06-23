#ifndef OS_EX3_LIST_H
#define OS_EX3_LIST_H

struct ListItem_t {
    int connFd;
    struct timeval arrivalTime;
};

struct List_t {
    struct ListItem_t* data;
    struct List_t* next;
};

typedef struct ListItem_t *ListItem;
typedef struct List_t *List;

List init();
List addNode(List list, int data, struct timeval arrivalTime, int* size);
ListItem removeFirst(List list, int* size);
void removeIndexes(List list, int* indexes_to_remove, int num_indexes, int* list_size, int* removed);
void deleteList(List list);
int is_in_array(int* array, int array_size, int value);
ListItem removeNode(List head, List nodeToRemove, int* list_size);

#endif //OS_EX3_LIST_H
