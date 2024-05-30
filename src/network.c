#include "network.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"

int setup_network_container() {
  debug("Bring up lo...\n");
  system("ip link set lo up");

  return 0;
}

int setup_network_host() {
  debug("Setting up network bridge\n");
  if (system("ip link show mini-container"))
    system("ip link add name mini-container type bridge");
  system("ip link set mini-container up");
  return 0;
}

int setup_hostname(const char *hostname) {
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