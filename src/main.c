#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "container.h"
#include "log.h"
#include "type.h"
#include "utils.h"

void usage(const char* name) {
  fprintf(stderr, "Usage: %s [options] image command [args]\n", name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -h, --help\t\tShow this help message and exit\n");
  fprintf(stderr, "  --hostname\t\tSet container hostname\n");
  fprintf(stderr, "  --rm\t\t\tRemove container when it exits\n");
  fprintf(stderr, "  --container-base\tSet container base path\n");
  fprintf(stderr, "  --cgroup-base\t\tSet cgroup base path\n");
  fprintf(stderr, "  -m,--memory\t\tSet memory limit in MB\n");
  fprintf(stderr, "  --cpus\t\tNumber of CPUs\n");
  fprintf(stderr, "  --cpuset-cpus\t\tCPUs in which to allow execution\n");
  fprintf(stderr, "  --cpu-weight\t\tSet CPU weight, ranges from 1 to 10000\n");
  fprintf(stderr, "  --cpu-max\t\tLimit max CPU usage in period of 1000000\n");
  fprintf(stderr, "  --debug\t\tPrint debug info\n");
  fprintf(stderr, "  -e, --env\t\tSet environment variables\n");
  exit(EXIT_SUCCESS);
}

void parse(int argc, char* argv[], container_config_t* config) {
  debug("Parsing arguments...\n");
  int opt, option_index;
  struct option long_options[] = {{"hostname", required_argument, 0, 0},
                                  {"rm", no_argument, 0, 0},
                                  {"container-base", required_argument, 0, 0},
                                  {"cgroup-base", required_argument, 0, 0},
                                  {"help", no_argument, 0, 'h'},
                                  {"debug", no_argument, 0, 0},
                                  {"env", required_argument, 0, 'e'},
                                  {"memory", required_argument, 0, 'm'},
                                  {"cpus", required_argument, 0, 0},
                                  {"cpuset-cpus", required_argument, 0, 0},
                                  {"cpu-weight", required_argument, 0, 0},
                                  {"cpu-max", required_argument, 0, 0},
                                  {0, 0, 0, 0}};
  while ((opt = getopt_long(argc, argv, "he:m:", long_options,
                            &option_index)) != -1) {
    switch (opt) {
      case 'h': {
        usage(argv[0]);
        break;
      }
      case 'e': {
        char* arg = strdup(optarg);
        char* key = strtok(optarg, "=");
        char* value = strtok(NULL, "=");
        if (key == NULL || value == NULL) {
          error("Invalid environment variable format: %s\n", arg);
          exit(EXIT_FAILURE);
        }
        free(arg);
        append(&config->env, key, value);
        break;
      }
      case 'm': {
        unsigned long long memory_limit = strtoull(optarg, NULL, 10);
        debug("Memory limit: %lld MB\n", memory_limit);
        char buf[50];
        snprintf(buf, 50, "%lld", memory_limit * 1024 * 1024);
        append(&config->cgroup_limit, "memory.max", buf);
        break;
      }
      case '?': {
        err(EXIT_FAILURE, "Invalid option argument: %s\n", argv[optind]);
        break;
      }
      case 0: {
        const char* option = long_options[option_index].name;
        debug("option: %s, optarg: %s\n", option, optarg);
        if (strcmp("hostname", option) == 0) {
          config->hostname = optarg;
        } else if (strcmp("rm", option) == 0) {
          config->rm = true;
        } else if (strcmp("container-base", option) == 0) {
          config->container_base = optarg;
        } else if (strcmp("cgroup-base", option) == 0) {
          config->cgroup_base_path = optarg;
        } else if (strcmp("debug", option) == 0) {
          set_log_level(LOG_DEBUG);
        } else if (strcmp("cpus", option) == 0) {
          char buf[50];
          int cpus = atoi(optarg);
          snprintf(buf, 50, "0-%d", cpus - 1);
          append(&config->cgroup_limit, "cpuset.cpus", buf);
        } else if (strcmp("cpuset-cpus", option) == 0) {
          append(&config->cgroup_limit, "cpuset.cpus", optarg);
        } else if (strcmp("cpu-weight", option) == 0) {
          append(&config->cgroup_limit, "cpu.weight", optarg);
        } else if (strcmp("cpu-max", option) == 0) {
          append(&config->cgroup_limit, "cpu.max", optarg);
        } else {
          err(EXIT_FAILURE, "Unknown option: %s\n", option);
        }
        break;
      }
      default: {
        usage(argv[0]);
        err(EXIT_FAILURE, "Unknown option: %c\n", opt);
        break;
      }
    }
  }
  if (optind > argc - 2) {
    error("Missing image path or command\n");
    usage(argv[0]);
  }
  config->image = argv[optind];
  config->args = argv + optind + 1;
  debug("Image: %s\n", config->image);
  debug("Command: %s\n", config->args[0]);
}

int main(int argc, char* argv[]) {
  debug("mini-container starting... PID: %ld\n", (long)getpid());
  debug("uid: %d, gid: %d\n", getuid(), getgid());

  struct container_config config = {
      .image_base_path = "/var/lib/mini-container/images",
      .container_base = "/var/lib/mini-container/volumns",
      .rm = true,
      .cgroup_base_path = "/sys/fs/cgroup/system.slice"};

  append(&config.env, "PATH", "/bin:/sbin:/usr/bin:/usr/sbin");
  append(&config.env, "HOME", "/root");
  append(&config.env, "USER", "root");
  append(&config.env, "TERM", "xterm-256color");
  debug("Environment variables:\n");
  for (pair_t* env = config.env; env; env = env->next) {
    debug("key: %s, value: %s\n", env->key, env->value);
  }
  parse(argc, argv, &config);
  run(&config);
  return 0;
}