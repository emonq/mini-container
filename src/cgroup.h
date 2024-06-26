#ifndef _CGROUP_H_
#define _CGROUP_H_

#include <sys/types.h>

#include "type.h"
int setup_cgroup(pid_t pid, const char *cgroup_base_path,
                 const char *container_id, const list_t *limits_list);
int cleanup_cgroup(const char *cgroup_base_path, const char *container_id);
#endif