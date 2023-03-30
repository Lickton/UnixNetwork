#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/log.h"

typedef int USER_ID;

/* User states */
#define UNDEF       0           /* undefined */
#define ONLINE      1           /* user connected with server */
#define OFFLINE     2           /* user not connected with server */

/* Misc manifest constants */
#define MAX_BUFFER  1024        /* max buffer size */
#define MAX_USER    20          /* at most MAX_USER instantaneous */
#define MAX_NAME    7           /* at most 7 characters */
#define PORT        55312				/* use this port to communicate */

/* Global variables */
const char userdata[] = ".irc/server/users";
struct user_t {                 /* the user struct */
  int   uid;                    /* user id */
  char  nick_name[MAX_NAME];    /* nick_name showed to other users */
  int   state;                  /* UNDEF, ONLINE, OFFLINE */
  int   cur_fd;                 /* fd to receive and send msg */
};
struct user_t users[MAX_USER];  /* The user list */
/* End global variables */

/* Function prototypes */
void initusers(char *userPath);

int safe_listen (int __fd, int __n);
int safe_socket(int __domain, int __type, int __protocol);
void safe_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
int safe_accept (int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len);

/* Pthead functions */
void *inform(char *msg, int uid);

/* 
 * main - The server main routine
 * */
int main(int argc, char *argv[])
{
  log_message(LOG_INFO, "Server starts");

  int server_sockfd = safe_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // bind socket
  struct sockaddr_in servAddr;
  bzero(&servAddr, sizeof(servAddr));
  servAddr.sin_family				= AF_INET;
  servAddr.sin_addr.s_addr 	= htonl(INADDR_ANY);
  servAddr.sin_port         = htons(PORT);

  safe_bind(server_sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr));

  // convert state to listen state
  safe_listen(server_sockfd, MAX_USER);

  // wait for connection
  struct sockaddr_in client_addr;
  socklen_t client_addr_len;

  while (1) {
    int client_socket = safe_accept(server_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    char buff[MAX_BUFFER];

    pid_t pid = fork();
    if (pid == 0) {
      /* child process */
      close(server_sockfd);

      int client_port;
      char client_in_info[MAX_BUFFER], client_out_info[MAX_BUFFER];
      const char *client_ipv4_addr;

      client_ipv4_addr = inet_ntop(AF_INET, &client_addr, buff, sizeof(buff));
      client_port = ntohs(client_addr.sin_port);

      sprintf(
          client_in_info,
          "Connection from %s, port %d established, process id %d",
          client_ipv4_addr, client_port, getpid()
      );
      log_message(LOG_INFO, client_in_info);

      // release socket and exit
      close(client_socket);
      sprintf(
          client_out_info,
          "Connection from %s, port %d closed, process id %d",
          client_ipv4_addr, client_port, getpid()
      );
      log_message(LOG_INFO, client_out_info);
      
      exit(EXIT_SUCCESS);

    } else {
      /* father process */

      close(client_socket);
    }
  }

  // control never reach here
  return EXIT_SUCCESS;
}

/*
 * Safe functions - encapsolution for functions
 */
int safe_socket (int __domain, int __type, int __protocol)
{
  int ret = socket(__domain, __type, __protocol);
  if (ret < 0) {
    log_message(LOG_ERROR, "Create socket failed");
    printf("Create socket failed\n");

    exit(EXIT_FAILURE);
  } else
    log_message(LOG_INFO, "Socket is created");

  return ret;
}

void safe_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
  int res = bind(__fd, __addr, __len);
  if (res < 0) {
    log_message(LOG_ERROR, "Bind failed");
    printf("Bind failed\n");

    exit(EXIT_FAILURE);
  } else
    log_message(LOG_INFO, "Address is bound");
}

int safe_listen (int __fd, int __n)
{
  int ret = listen(__fd, __n);
  if (ret < 0) {
    log_message(LOG_ERROR, "Listen failed");
    printf("Listen failed\n");

    exit(EXIT_FAILURE);
  } else
    log_message(LOG_INFO, "Listen state");

  return ret;
}

int safe_accept (int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len)
{
  int ret = accept(__fd, __addr, __addr_len);
  if (ret < 0) {
    log_message(LOG_ERROR, "Accept client connection failed");
    printf("Accept client connection failed\n");

    exit(EXIT_FAILURE);
  }

  return ret;
}
