#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/log.h"

/* User states */
#define UNDEF       0           /* undefined */
#define ONLINE      1           /* user connected with server */
#define OFFLINE     2           /* user not connected with server */

/* Misc manifest constants */
#define MAX_BUFFER  1024        /* max buffer size */
#define MAX_NAME    7           /* at most 7 characters */
#define MAX_USERS   40          /* at most MAX_USERS instantaneous */
#define PORT        55312       /* use this port to communicate */

/* Global variables */
int nextuid = 1;                /* next user ID to allocate */
struct user_t {                 /* the user struct */
  int   uid;                    /* user id */
  pthread_t tid;                /* process ID */
  char  *name;                  /* name showed to other users */
  char  *ipv4_addr;             /* user's ipv4 address */
  int   port;                   /* user's port */
  int   fd;                     /* fd to receive and send msg */
};
struct user_t users[MAX_USERS]; /* The user list */
pthread_mutex_t user_list_mutex = PTHREAD_MUTEX_INITIALIZER; /* user list mutex */
/* End global variables */

/* Function prototypes */
void initusers();
void clearuser(struct user_t *user);
int maxuid(struct user_t *users);
int adduser(struct user_t *user);
int deleteuser(struct user_t *user, pthread_t tid);

int safe_listen (int __fd, int __n);
int safe_socket(int __domain, int __type, int __protocol);
void safe_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);
int safe_accept (int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len);

/* communication functions */
void forward(char *__msg);
void *request_hanlder(void *__user);

/* main - The server main routine */
int main(int argc, char *argv[])
{
  log_message(LOG_INFO, "Server starts");

  initusers();

  int server_sockfd = safe_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // bind socket
  struct sockaddr_in servAddr;
  bzero(&servAddr, sizeof(servAddr));
  servAddr.sin_family				= AF_INET;
  servAddr.sin_addr.s_addr 	= htonl(INADDR_ANY);
  servAddr.sin_port         = htons(PORT);

  safe_bind(server_sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr));

  // convert state to listen state
  safe_listen(server_sockfd, MAX_USERS);

  // wait for connection
  struct sockaddr_in client_addr;
  socklen_t client_addr_len;

  while (1) {
    int client_socket = safe_accept(
        server_sockfd, (struct sockaddr*)&client_addr,
        &client_addr_len);
    char buff[MAX_BUFFER];

    pthread_t th;
    struct user_t user;
    user.fd = client_socket;
    user.ipv4_addr = (char *)inet_ntop(AF_INET, &client_addr.sin_addr, buff, sizeof(buff));
    user.port = ntohs(client_addr.sin_port);
    user.name = "test";
    user.tid = *(&th);
    log_message(LOG_DEBUG, "User info created");

    pthread_create(&th, NULL, request_hanlder, (void *)&user);
  }

  log_message(LOG_DEBUG, "Server closed");
  close(server_sockfd);
  return EXIT_SUCCESS;
}

/**********************************
 * Socket communication functions
 *********************************/

/* handle the request sent by user */
void *request_hanlder(void *__user)
{
  log_message(LOG_DEBUG, "Come into request_hanlder");
  struct user_t *user = (struct user_t *)__user;
  char buff[MAX_BUFFER];
  adduser(__user);
  log_message(LOG_DEBUG, "Leave adduser");

  while (1) {
    bzero(buff, sizeof(buff));
    read(user->fd, buff, sizeof(buff));
    buff[strlen(buff) - 1] = '\0';
    log_message(LOG_INFO, "Receive message");
    forward(buff);
  }

  return NULL;
}

/* send messages to all other users */
void forward(char *__msg)
{
  char *msg = (char *)__msg;

  pthread_mutex_lock(&user_list_mutex);
  for (int i = 0; i < MAX_USERS; i++) {
    if (users[i].fd != -1) {
      write(users[i].fd, msg, MAX_BUFFER);
      char info[1024];
      sprintf(info, "sending message to user:%d, \"%s\"", users[i].uid, msg);
      log_message(LOG_DEBUG, info);
    }
  }
  pthread_mutex_unlock(&user_list_mutex);
}

/******************************************
 * Routines that manipulate the user list
 *****************************************/

/* initsuers - Initialize the user list */
void initusers()
{
  for (int i = 0; i < MAX_USERS; i++) {
    users[i].uid        = 0;
    users[i].tid        = 0;
    users[i].name       = "\0";
    users[i].ipv4_addr  = "\0";
    users[i].port       = 0;
    users[i].fd         = -1;
  }

  log_message(LOG_INFO, "User list initialized");
}

/* clearuser - Clear the entries in a job struct */
void clearuser(struct user_t *user)
{
  char clearinfo[MAX_BUFFER];
  sprintf(clearinfo, "User from %s:%d canceled. uid:%d", user->ipv4_addr, user->port, user->uid);
  log_message(LOG_INFO, clearinfo);

  user->uid = 0;
  user->name = "\0";
  close(user->fd);
  user->fd = -1;
  user->port = 0;
  user->ipv4_addr = "\0";
  user->tid = 0;
}

/* maxuid - Returns largest allocated user ID */
int maxuid(struct user_t *user)
{
  int max = 0;
  pthread_mutex_lock(&user_list_mutex);
  for (int i = 0; i < MAX_USERS; i++) {
    if (users[i].uid > max)
      max = users[i].uid;
  }
  pthread_mutex_unlock(&user_list_mutex);
  return max;
}

/* adduser - Add a user to the user list */
int adduser(struct user_t *user)
{
  for (int i = 0; i < MAX_USERS; i++) {
    if (users[i].uid == 0) {
      /* mutex lock */
      log_message(LOG_DEBUG, "Wait for mutex");
      pthread_mutex_lock(&user_list_mutex);
      log_message(LOG_DEBUG, "Gain the mutex");
      users[i].fd   = user->fd;
      users[i].uid  = nextuid++;
      users[i].tid  = user->tid;
      users[i].port = user->port;
      if (nextuid > MAX_USERS)
        nextuid = 1;
      users[i].name = user->name;
      users[i].ipv4_addr = user->ipv4_addr;
      log_message(LOG_DEBUG, "Finish copy of info");

      char info[MAX_BUFFER];
      sprintf(info, "User from %s:%d registered. tid:%lu", user->ipv4_addr, user->port, users[i].tid);

      /* mutex unlock */
      pthread_mutex_unlock(&user_list_mutex);
      log_message(LOG_DEBUG, "Leave the mutex");

      log_message(LOG_INFO, info);
      return 0;
    }
  }

  log_message(LOG_WARNING, "Too much users right now");
  return 1;
}

/* deleteuser - Delete a job from the user list */
int deleteuser(struct user_t *user, pthread_t tid)
{
  for (int i = 0; i < MAX_USERS; i++) {
    if (users[i].tid == tid) {
      clearuser(&users[i]);
      nextuid = maxuid(users) + 1;
      return 1;
    }
  }

  return 0;
}
 
/***********************************************
 * Safe functions - encapsolution for functions
 **********************************************/

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
