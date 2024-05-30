#ifndef _USER_H_
#define _USER_H_

#include <sys/types.h>

int setup_user_mapping(pid_t child_pid, uid_t uid, gid_t gid, int socket_fd);

#endif