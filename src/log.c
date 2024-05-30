#include "log.h"

#include <stdarg.h>
#include <stdio.h>

log_level_t log_level = LOG_INFO;

void set_log_level(log_level_t level) { log_level = level; }

#define LOG_FUNC(level, level_upper)           \
  void level(const char *fmt, ...) {           \
    if (log_level > LOG_##level_upper) return; \
    fprintf(stderr, "[" #level_upper "] ");    \
    va_list args;                              \
    va_start(args, fmt);                       \
    vfprintf(stderr, fmt, args);               \
    va_end(args);                              \
  }

LOG_FUNC(debug, DEBUG)
LOG_FUNC(info, INFO)
LOG_FUNC(warn, WARN)
LOG_FUNC(error, ERROR)