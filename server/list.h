#ifndef LIST_H
#define LIST_H

#include <pthread.h>

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    size_t data_size;
    pthread_mutex_t lock;
} List;

List* create_list(size_t data_size);
void add_to_list(List* list, void* data);
void destroy_list(List* list);

#endif

