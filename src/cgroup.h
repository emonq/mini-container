#ifndef _CGROUP_H_
#define _CGROUP_H_

#include "type.h"
int setup_cgroup(const char *cgroup_base_path, const char *container_id,
                 const pair_t *limits);
int cleanup_cgroup(const char *cgroup_base_path, const char *container_id);
#endif