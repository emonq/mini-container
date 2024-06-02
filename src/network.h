#ifndef _NETWORK_H_
#define _NETWORK_H_
#include <sys/types.h>

int setup_network_container(const char* id, const pid_t pid, const char* ip, const char* gateway);

int setup_network_host();

int setup_hostname(const char* hostname);

#endif