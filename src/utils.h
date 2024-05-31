#ifndef _UTILS_H_
#define _UTILS_H_
#include <syscall.h>

#define clone3(args) syscall(SYS_clone3, args, sizeof(struct clone_args))
#define pivot_root(new_root, put_old) syscall(SYS_pivot_root, new_root, put_old)

int rm_rf(const char *path);

char *sha256_string(const char *data, size_t size, char *sha256);

unsigned long long timestamp();

#endif