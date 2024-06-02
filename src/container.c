#define _GNU_SOURCE
#include "container.h"

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/sched.h>
#include <openssl/sha.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
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

#define STACK_SIZE (1024 * 1024)

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

  if (setup_filesystem(image_path, config->id, config->container_base,
                       config->mounts) ||
      setup_hostname(config->hostname)) {
    error("Error initializing container, exiting...\n");
    return 1;
  }
  debug("Starting container...\n");
  clearenv();

  debug("Setting env...\n");
  for (list_t *node = config->env; node; node = node->next) {
    pair_t *env = (pair_t *)node->data;
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

  prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
  char *container_stack;
  container_stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  pid_t child_pid =
      clone(container_init, container_stack + STACK_SIZE,
            CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET |
                CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWCGROUP | SIGCHLD,
            config);
  if (child_pid == -1) err(EXIT_FAILURE, "clone");

  debug("Child PID: %ld\n", (long)child_pid);

  setup_cgroup(child_pid, config->cgroup_base_path, config->id,
               config->cgroup_limit);

  int uid = getuid(), gid = getgid();
  setup_user_mapping(child_pid, uid, gid, sockets[0]);
  setup_network_container(config->id, child_pid, config->ip, config->gateway);

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
