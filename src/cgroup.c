#include "cgroup.h"

#include <err.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "type.h"
#include "utils.h"

int setup_limits(const char *cgroup_path, const pair_t *limits) {
  for (const pair_t *limit = limits; limit; limit = limit->next) {
    if (strlen(limit->key) >= 50) {
      error("Cgroup limit key too long\n");
      exit(EXIT_FAILURE);
    }
    debug("Setting cgroup limit: %s=%s\n", limit->key, limit->value);
    char limit_path[PATH_MAX + 50];
    snprintf(limit_path, PATH_MAX + 50, "%s/%s", cgroup_path, limit->key);
    int fd = open(limit_path, O_WRONLY);
    if (fd == -1) err(EXIT_FAILURE, "open-cgroup_limit %s", limit_path);
    if (write(fd, limit->value, strlen(limit->value)) == -1)
      err(EXIT_FAILURE, "Set cgroup limit %s to %s", limit->key, limit->value);
    close(fd);
  }
  return 0;
}

int setup_cgroup(const char *cgroup_base_path, const char *container_id,
                 const struct pair *limits) {
  char cgroup_path[PATH_MAX];
  snprintf(cgroup_path, PATH_MAX, "%s/%s", cgroup_base_path, container_id);
  debug("Cgroup path: %s\n", cgroup_path);
  if (mkdir(cgroup_path, 0700))
    err(EXIT_FAILURE, "mkdir-cgroup %s", cgroup_path);
  if (limits) setup_limits(cgroup_path, limits);
  return 0;
}

int cleanup_cgroup(const char *cgroup_base_path, const char *container_id) {
  char cgroup_path[PATH_MAX];
  snprintf(cgroup_path, PATH_MAX, "%s/%s", cgroup_base_path, container_id);
  debug("Cleaning up cgroup path: %s\n", cgroup_path);
  if (rmdir(cgroup_path)) err(EXIT_FAILURE, "rmdir-cgroup_path");
  return 0;
}
