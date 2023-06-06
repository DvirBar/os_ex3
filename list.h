#ifndef OS_EX3_LIST_H
#define OS_EX3_LIST_H

typedef struct List_t *List;

List init();
List addNode(List list, int data, int* size);
int removeFirst(List list, int* size);
void deleteList(List list);

#endif //OS_EX3_LIST_H
