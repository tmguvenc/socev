#ifndef LIB_LINKED_LIST_H_
#define LIB_LINKED_LIST_H_

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

typedef void* data_type;
typedef int key_type;

typedef struct node {
  data_type data;
  key_type key;
  struct node *next;
} node_t;

void add_to_list(node_t **head, key_type key, data_type data) {
  if (!*head) {
    *head = (node_t *)malloc(sizeof(node_t));
    (*head)->key = key;
    (*head)->data = data;
    (*head)->next = NULL;
  } else {
    node_t *current = *head;
    while (current->next != NULL) {
      current = current->next;
    }

    current->next = (node_t *)malloc(sizeof(node_t));
    current->next->key = key;
    current->next->data = data;
    current->next->next = NULL;
  }
}

void print_list(node_t *head) {
  node_t *current = head;

  while (current != NULL) {
    printf("%d: %d\n", current->key, current->data);
    current = current->next;
  }
}

void delete_from_list(node_t **head, key_type key) {
  node_t *curr = *head;

  if (curr && curr->key == key) {
    node_t *del = *head;
    *head = curr->next;
    free(del);
    return;
  }

  node_t *prev;
  while (curr && curr->key != key) {
    prev = curr;
    curr = curr->next;
  }

  if (curr == NULL) {
    printf("%d not found\n", key);
    return;
  }

  prev->next = curr->next;

  free(curr);
}

#endif // LIB_LINKED_LIST_H_
