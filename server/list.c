#include <stdlib.h>
#include <string.h>
#include "list.h"

List* create_list(size_t data_size) {
    List* list = (List*)malloc(sizeof(List));
    list->head = NULL;
    list->tail = NULL;
    list->data_size = data_size;
    pthread_mutex_init(&list->lock, NULL);
    return list;
}

void add_to_list(List* list, void* data) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = malloc(list->data_size);
    memcpy(node->data, data, list->data_size);
    node->next = NULL;

    pthread_mutex_lock(&list->lock);
    if (list->tail == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    pthread_mutex_unlock(&list->lock);
}

void destroy_list(List* list) {
    pthread_mutex_lock(&list->lock);
    Node* current = list->head;
    while (current) {
        Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    pthread_mutex_unlock(&list->lock);
    pthread_mutex_destroy(&list->lock);
    free(list);
}

