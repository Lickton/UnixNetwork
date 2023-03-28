#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/log.h"

int main()
{
  log_message(LOG_INFO, "Server starts");
  return EXIT_SUCCESS;
}
