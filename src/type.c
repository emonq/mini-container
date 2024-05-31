#include "type.h"

#include <stdlib.h>
#include <string.h>

list_t *append_pair(list_t **head, const char *key, const char *value) {
  pair_t *new = malloc(sizeof(pair_t));
  list_t *node = malloc(sizeof(list_t));
  new->key = strdup(key);
  new->value = strdup(value);
  node->data = new;
  node->next = NULL;
  if (*head != NULL) {
    list_t *cur = *head;
    while (cur->next) cur = cur->next;
    cur->next = node;
  } else
    *head = node;
  return node;
}