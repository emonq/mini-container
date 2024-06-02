#include "user.h"

#include <err.h>
#include <linux/limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"

int setup_user_mapping(pid_t child_pid, uid_t uid, gid_t gid) {
  debug("Setting up user map for child %ld with uid=%d, gid=%d...\n",
        (long)child_pid, uid, gid);
  char path[PATH_MAX];
  sprintf(path, "/proc/%ld/uid_map", (long)child_pid);
  FILE *uid_map = fopen(path, "w");
  if (uid_map == NULL) err(EXIT_FAILURE, "fopen-uid_map");
  fprintf(uid_map, "0 %d 1\n", uid);
  fclose(uid_map);

  // since Linux 3.19, gid map is only writable by unprivileged process when
  // setgroups is disabled
  sprintf(path, "/proc/%ld/setgroups", (long)child_pid);
  FILE *setgroups = fopen(path, "w");
  if (setgroups == NULL) err(EXIT_FAILURE, "fopen-setgroups");
  fprintf(setgroups, "deny");
  fclose(setgroups);

  sprintf(path, "/proc/%ld/gid_map", (long)child_pid);
  FILE *gid_map = fopen(path, "w");
  if (gid_map == NULL) err(EXIT_FAILURE, "fopen-gid_map");
  fprintf(gid_map, "0 %d 1\n", gid);
  fclose(gid_map);

  return 0;
}