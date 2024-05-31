#ifndef _CONTAINER_H_
#define _CONTAINER_H_
#include <linux/limits.h>
#include <stdbool.h>
#include <syscall.h>
#include <unistd.h>

#include "type.h"

#define CONTAINER_ID_LEN_MAX 64
#define CONTAINER_SUFFIX_LEN_MAX 100
#define CONTAINER_HOSTNAME_LEN_MAX 253

struct container_config {
  char *image_base_path;
  char *image;
  char *container_base;
  char *hostname;
  char *cgroup_base_path;
  char *id;
  list_t *cgroup_limit;
  bool rm;
  int fd;
  list_t *env;
  struct mount *mounts;
  char **args;
};

typedef struct container_config container_config_t;

void run(struct container_config *config);

#endif