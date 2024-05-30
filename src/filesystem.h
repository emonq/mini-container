#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#define MOUNT_POINT_LEN_MAX 256

struct mount_options {
  char *source;
  char *target;
  char *filesystem;
  unsigned long flags;
  char *data;
};

int setup_filesystem(const char *image_path, const char *container_id,
                     const char *container_base);

#endif