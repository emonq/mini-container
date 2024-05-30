#ifndef _TYPE_H_
#define _TYPE_H_

struct pair {
  char *key;
  char *value;
  struct pair *next;
};

typedef struct pair pair_t;

pair_t *append(pair_t **head, const char *key, const char *value);

#endif
