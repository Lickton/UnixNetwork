#ifndef LOG_H
#define LOG_H

#define BUFFER_SIZE 1024

/* const varibles */
const char LOG_FILE[] = "/var/log/IRC-server.log";    /* LOG FILE location    */
typedef enum {                                        /* Level of information */
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR
} log_level_t;

/* log routines */
void log_message(log_level_t level, const char *info);
char *message(log_level_t level, const char *info);

#endif
