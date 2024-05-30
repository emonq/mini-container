#include <dirent.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

int rm_rf(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      DIR *dir = opendir(path);
      if (dir == NULL) {
        perror("opendir");
        return -1;
      }
      struct dirent *entry;
      while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
          continue;
        }
        char *new_path =
            (char *)malloc(strlen(path) + strlen(entry->d_name) + 2);
        sprintf(new_path, "%s/%s", path, entry->d_name);
        if (rm_rf(new_path) != 0) {
          return -1;
        }
        free(new_path);
      }
      closedir(dir);
      if (rmdir(path) != 0) {
        perror("rmdir");
        return -1;
      }
    } else {
      if (unlink(path) != 0) {
        perror("unlink");
        return -1;
      }
    }
  } else {
    perror("stat");
    return -1;
  }
  return 0;
}

char *sha256_string(const char *data, size_t size, char *sha256) {
  unsigned char buf[SHA256_DIGEST_LENGTH + 1];
  SHA256((unsigned char *)data, strlen(data), buf);
  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf(sha256 + i * 2, "%02x", buf[i]);
  }
  return sha256;
}

unsigned long long timestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000LL + tv.tv_usec;
}