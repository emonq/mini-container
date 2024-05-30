#ifndef _LOG_H_
#define _LOG_H_

enum log_level { LOG_DEBUG = 0, LOG_INFO, LOG_WARN, LOG_ERROR };

typedef enum log_level log_level_t;

void set_log_level(log_level_t level);
void debug(const char *fmt, ...);
void info(const char *fmt, ...);
void warn(const char *fmt, ...);
void error(const char *fmt, ...);

#endif