#ifndef _TYPE_H_
#define _TYPE_H_

struct linked_list {
  void *data;
  struct linked_list *next;
};

struct pair {
  char *key;
  char *value;
};

typedef struct pair pair_t;
typedef struct linked_list list_t;

list_t *append(list_t **head, void *data);
list_t *append_pair(list_t **head, const char *key, const char *value);

#endif
