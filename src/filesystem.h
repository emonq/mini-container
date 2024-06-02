#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_
#include "type.h"
#define MOUNT_POINT_LEN_MAX 256

struct mount_options {
  char *source;
  char *target;
  char *filesystem;
  unsigned long flags;
  char *data;
};

list_t *append_mount_options(list_t **head, const char *source,
                             const char *target, const char *filesystem,
                             unsigned long flags, const char *data);

int setup_filesystem(const char *image_path, const char *container_id,
                     const char *container_base, list_t *mounts);

int parse_bind_mount_option(const char *options);

#endif