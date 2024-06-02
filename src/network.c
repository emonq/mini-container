#include "network.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"

int setup_network_container(const char* id, const pid_t pid, const char* ip,
                            const char* gateway) {
  debug("Bring up lo...\n");
  system("ip link set lo up");
  if (ip == NULL || gateway == NULL) return 1;
  char id_short[10];
  snprintf(id_short, 6, "%s", id);
  debug("id_short %s\n", id_short);
  char cmd[1024];
  snprintf(cmd, 1024, "./scripts/setup_veth.sh %s %ld %s %s", id_short,
           (unsigned long)pid, ip, gateway);
  system(cmd);

  return 0;
}

int setup_network_host() {
  debug("Setting up network bridge\n");
  if (system("ip link show mini-container"))
    system("ip link add name mini-container type bridge");
  system("ip link set mini-container up");
  return 0;
}

int setup_hostname(const char* hostname) {
  if (hostname == NULL) {
    error("Hostname is not set\n");
    return 1;
  }
  debug("Setting hostname to %s...\n", hostname);
  if (hostname == NULL) return 1;
  int status;
  if ((status = sethostname(hostname, strlen(hostname))) == -1)
    err(EXIT_FAILURE, "sethostname");
  return 0;
}