#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <errno.h>
#include <unistd.h>
#include <time.h>

#include "../include/log.h"

/* message - Return a message formated in [time][level]info\t (errno) */
char *message(log_level_t level, const char *info)
{
  const char *level_str;

  switch (level) {
    case LOG_DEBUG:
      level_str = "DEBUG";
      break;
    case LOG_INFO:
      level_str = "INFO";
      break;
    case LOG_WARNING:
      level_str = "WARNING";
      break;
    case LOG_ERROR:
      level_str = "ERROR";
      break;
    default:
      level_str = "UNKNOWN";
      break;
  }

  char *result = malloc(BUFFER_SIZE * sizeof(char));
  sprintf(result, "[%s][%s]%s", ctime(NULL), level_str, info);

  if (level == LOG_ERROR) {
    char error_message[BUFFER_SIZE];
    sprintf(error_message, "\t errno:%d", errno);
    strcat(result, error_message);
  }

  strcat(result, "\n");

  return result;
}

/* log_message - writing logs to LOG_FILE */
void log_message(log_level_t level, const char *info)
{
  FILE *log_file = fopen(LOG_FILE, "a");
  if (log_file == NULL) {
    printf("open log file at %s failed. errno:%d\n", LOG_FILE, errno);;
    exit(EXIT_FAILURE);
  }

  char *str = message(level, info);

  int statu = fwrite(str, sizeof(str), 1, log_file);
  if (statu != sizeof(str)) {
    perror("writing log file error\n");
    exit(EXIT_FAILURE);
  }
  

  fclose(log_file);
}
