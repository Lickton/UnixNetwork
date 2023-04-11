#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "../include/log.h"

const char IRC_DIR[]  = ".irc";
const char LOG_DIR[]  = "log";
const char LOG_FILE[] = "client.log";    /* LOG FILE location    */

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

  time_t ticks = time(NULL);
  char *cur_time = ctime(&ticks);
  cur_time[strlen(cur_time) - 1] = '\0';

  char *result = malloc(BUFFER_SIZE);
  sprintf(result, "[%s][%s] %s", cur_time, level_str, info);

  if (level == LOG_ERROR) {
    char error_message[BUFFER_SIZE];
    sprintf(error_message, " errno:%d", errno);
    strcat(result, error_message);
  }

  strcat(result, "\n");

  return result;
}

FILE *open_file()
{
  char *home = getenv("HOME");
  char path_dir[strlen(IRC_DIR) + strlen(home) + 2];

  /* path_dir = ~/.irc */
  sprintf(path_dir, "%s/%s", home, IRC_DIR);
  if (access(path_dir, F_OK) != 0) {
    if (mkdir(path_dir, 0777) == -1)
      perror("Create IRC_DIR failed");
  }

  /* path_log = ~/.irc/log */
  char path_log[strlen(path_dir) + strlen(LOG_DIR) + 2];
  sprintf(path_log, "%s/%s", path_dir, LOG_DIR);
  if (access(path_log, F_OK) != 0) {
    if (mkdir(path_log, 0777) == -1)
      perror("Create LOG_DIR failed");
  }

  /* path_file = ~/.irc/log/server.log */
  char path_file[strlen(path_log) + strlen(LOG_FILE) + 2];
  sprintf(path_file, "%s/%s", path_log, LOG_FILE);

  FILE *log_file = fopen(path_file, "ab");
  return log_file;
}

/* log_message - writing logs to LOG_FILE */
void log_message(log_level_t level, const char *info)
{
  FILE *log_file = open_file();

  if (log_file == NULL) {
    printf("open log file failed. errno:%d\n", errno);;
    exit(EXIT_FAILURE);
  }

  char *str = message(level, info);

  fprintf(log_file, "%s", str);

  fclose(log_file);
}
