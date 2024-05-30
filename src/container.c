#define _GNU_SOURCE
#include "container.h"

#include <err.h>
#include <limits.h>
#include <linux/sched.h>
#include <openssl/sha.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cgroup.h"
#include "filesystem.h"
#include "log.h"
#include "network.h"
#include "type.h"
#include "user.h"
#include "utils.h"

char *gen_id(char *id, struct container_config *config) {
  unsigned long long now = timestamp();
  if (config->hostname && strlen(config->hostname) > CONTAINER_HOSTNAME_LEN_MAX)
    err(EXIT_FAILURE, "hostname too long");
  char data[1024];
  if (config->hostname)
    snprintf(data, 1024, "%s%lld", config->hostname, now);
  else
    snprintf(data, 1024, "%lld", now);
  debug("Generating ID: %s\n", data);
  sha256_string(data, strlen(data), id);
  return id;
}

void cleanup(struct container_config *config) {
  cleanup_cgroup(config->cgroup_base_path, config->id);
  char container_data_path[PATH_MAX];
  snprintf(container_data_path, PATH_MAX, "%s/%s", config->container_base,
           config->id);
  debug("Cleaning up container data path: %s\n", container_data_path);
  if (rm_rf(container_data_path))
    err(EXIT_FAILURE, "rmdir-container_data_path: %s", container_data_path);
}

static int container_init(void *args) {
  struct container_config *config = (struct container_config *)args;
  int socket_fd = config->fd;

  // wait for user map setup
  int res;
  if (read(socket_fd, &res, sizeof(int)) != sizeof(int))
    err(EXIT_FAILURE, "read from setup_user_map");
  close(socket_fd);
  debug("User map setup completed\n");

  if (strlen(config->id) > CONTAINER_ID_LEN_MAX) {
    error("Container ID too long\n");
    exit(EXIT_FAILURE);
  }

  char image_path[PATH_MAX];
  if (strlen(config->image_base_path) + strlen(config->image) + 10 > PATH_MAX) {
    error("Image path too long\n");
    exit(EXIT_FAILURE);
  }
  snprintf(image_path, PATH_MAX, "%s/%s/rootfs", config->image_base_path,
           config->image);

  if (setup_filesystem(image_path, config->id, config->container_base) ||
      setup_hostname(config->hostname) || setup_network_container()) {
    error("Error initializing container, exiting...\n");
    return 1;
  }
  debug("Starting container...\n");
  clearenv();

  debug("Setting env...\n");
  for (pair_t *env = config->env; env; env = env->next) {
    debug("Setting env: %s=%s\n", env->key, env->value);
    if (setenv(env->key, env->value, 1) == -1)
      err(EXIT_FAILURE, "setenv %s=%s", env->key, env->value);
  }

  char **cmd = config->args;

  debug("Running command: %s...\n", cmd[0]);
  execvp(cmd[0], cmd);

  // error occurred
  err(EXIT_FAILURE, "Error running command %s", cmd[0]);
}

void run(struct container_config *config) {
  debug("Running container...\n");
  char *id = malloc(CONTAINER_ID_LEN_MAX + 1);
  gen_id(id, config);
  config->id = id;
  debug("Container ID: %s\n", config->id);
  // set hostname to container ID if not set
  if (config->hostname == NULL) config->hostname = id;

  int sockets[2];
  if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, sockets))
    err(EXIT_FAILURE, "socketpair");
  config->fd = sockets[1];
  if (fcntl(sockets[0], F_SETFD, FD_CLOEXEC) == -1) err(EXIT_FAILURE, "fcntl");
  setup_cgroup(config->cgroup_base_path, config->id, config->cgroup_limit);
  char cgroup_path[PATH_MAX];
  snprintf(cgroup_path, PATH_MAX, "%s/%s", config->cgroup_base_path,
           config->id);
  int cgroup_fd = open(cgroup_path, O_RDONLY);
  if (cgroup_fd == -1) err(EXIT_FAILURE, "open-cgroup");

  struct clone_args clargs = {
      .flags = CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET |
               CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWCGROUP |
               CLONE_INTO_CGROUP,
      .exit_signal = SIGCHLD,
      .cgroup = cgroup_fd,
  };
  prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
  pid_t child_pid = clone3(&clargs);
  if (child_pid == -1) err(EXIT_FAILURE, "clone");
  close(cgroup_fd);
  if (child_pid == 0) {
    container_init(config);
    exit(1);
  }

  debug("Child PID: %ld\n", (long)child_pid);
  int uid = getuid(), gid = getgid();
  setup_user_mapping(child_pid, uid, gid, sockets[0]);

  // wait for child process to terminate
  int status;
  waitpid(child_pid, &status, 0);
  if (WIFEXITED(status))
    info("Child exited with status %d\n", WEXITSTATUS(status));
  else if (WIFSIGNALED(status))
    error("Child killed by signal %d\n", WTERMSIG(status));
  else
    error("Child exited with unknown status\n");
  if (config->rm) cleanup(config);
}
