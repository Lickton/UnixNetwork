#ifndef LOG_H
#define LOG_H

#define BUFFER_SIZE 1024 

/* const varibles */
typedef enum {                                        /* Level of information */
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR
} log_level_t;

/* log routines */
void log_message(log_level_t level, const char *info);

#endif
