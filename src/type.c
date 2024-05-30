#include "type.h"

#include <stdlib.h>
#include <string.h>

pair_t *append(pair_t **head, const char *key, const char *value) {
  pair_t *new = malloc(sizeof(pair_t));
  new->key = strdup(key);
  new->value = strdup(value);
  new->next = NULL;
  if (*head != NULL) {
    pair_t *cur = *head;
    while (cur->next) cur = cur->next;
    cur->next = new;
  } else
    *head = new;
  return new;
}