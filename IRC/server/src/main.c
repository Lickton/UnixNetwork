#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../include/log.h"

/* Misc manifest constants */
#define PORT 						55312
#define MAX_USERS 			50
#define MAX_NAME_LEM 		20
#define MAX_MSG_LEN 		256
#define MAX_MSG_NUM 		100
#define MAX_BUFFER 			1024

/* Global varibles begins */
typedef enum {
  REGISTE,
  SEND_PUBLIC,
  SEND_PRIVATE,
  QUIT
} command_option;

typedef enum {
  SESSION_ONE,
  SESSION_TWO,
  SESSION_THREE
} screen_num;

typedef struct message {
  time_t timestamp;
  int uid_from;
  int uid_to;
  char user_name[MAX_NAME_LEM];
  char content[MAX_MSG_LEN];
} message_t;

struct message_encode {
  screen_num screem;
  struct message message_t;
};

struct message_decode {
  command_option opt;
  struct message message_t;
};

typedef struct user {
  int uid;
  char user_name[MAX_NAME_LEM];
  int sockfd;
} user_t;

typedef struct node {
  user_t user;
  struct node *next;
} node_t;

typedef struct queue {
  node_t *front;
  node_t *rear;
  int size;
} queue_t;
queue_t *users;

typedef struct message_queue {
  struct message_decode *messages[MAX_MSG_NUM];
  int front;
  int rear;
  int size;
} message_queue_t;
message_queue_t *messages;

pthread_mutex_t users_mutex_lock;
pthread_mutex_t message_mutex_lock;
pthread_cond_t user_change_cond;

int next_uid;
/* Global varibles ends */

/* Function prototypes */
void *handle_client(void *arg);
void *send_msg();
void *broadcast_users();
void enqueue(queue_t *q, user_t user);
void dequeue(queue_t *q, int uid);
void enqueue_message(message_queue_t *q, struct message_decode *message);
struct message_decode *dequeue_message(message_queue_t *q);

int main(int argc, char *argv[])
{
  log_message(LOG_INFO, "Server starts");

  int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_fd < 0) {
    log_message(LOG_ERROR, "create socket error");
    return -1;
  }
  log_message(LOG_INFO, "Socket created");

  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family 			= AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port 				= htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    log_message(LOG_ERROR, "Bind error");
    return -1;
  }
  log_message(LOG_INFO, "Bind port:55312");

  if (listen(server_fd, MAX_USERS) == -1) {
    log_message(LOG_ERROR, "Listen failed");
    return -1;
  }
  log_message(LOG_INFO, "Listen state");

  users = (queue_t*)malloc(sizeof(queue_t));
  users->size = 0;
  users->front = users->rear = NULL;

  messages = (message_queue_t*)malloc(sizeof(message_queue_t));
  messages->front = messages->rear = messages->size = 0;

  pthread_mutex_init(&users_mutex_lock, NULL);
  pthread_mutex_init(&message_mutex_lock, NULL);
  pthread_cond_init(&user_change_cond, NULL);

  pthread_t comsumer_tid, broadcast_tid;
  pthread_create(&comsumer_tid, NULL, send_msg, NULL);
  pthread_create(&broadcast_tid, NULL, broadcast_users, NULL);

  struct sockaddr_in client_addr;
  socklen_t len;

  while (1) {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if (client_fd == -1) {
      log_message(LOG_ERROR, "Accept failed");
      exit(EXIT_FAILURE);
    }

    log_message(LOG_INFO, "Accepted");

    pthread_t tid;
    if (pthread_create(&tid, NULL, handle_client, (void *)&client_fd) != 0) {
      log_message(LOG_ERROR, "Create thread failed");
      exit(EXIT_FAILURE);
    }
  }

  close(server_fd);
  return EXIT_SUCCESS;
}

/*********************
 * Pthread Funcitons
 ********************/

void *handle_client(void *arg)
{
  int client_fd = *(int *)arg;

  user_t user;

  int online = 1;
  char info[MAX_BUFFER];

  while (online) {
    // log_message(LOG_DEBUG, "Still online");

    struct message_decode msg;
    bzero(info, sizeof(info));
    read(client_fd, (void *)&msg, sizeof(msg));
    // log_message(LOG_DEBUG, "received info");

    switch (msg.opt) {
      case REGISTE:
        pthread_mutex_lock(&users_mutex_lock);

        strcpy(user.user_name, msg.message_t.user_name);
        user.sockfd = client_fd;
        user.uid = ++next_uid;
        write(client_fd, (void*)&user.uid, sizeof(user.uid));
        enqueue(users, user);

        pthread_mutex_unlock(&users_mutex_lock);
        break;
      case SEND_PRIVATE:
      case SEND_PUBLIC:
        pthread_mutex_lock(&message_mutex_lock);
        
    		enqueue_message(messages, &msg);

        pthread_mutex_unlock(&message_mutex_lock);
        break;
     case QUIT:
        pthread_mutex_lock(&users_mutex_lock);

        dequeue(users, user.uid);

        pthread_mutex_unlock(&users_mutex_lock);
        sprintf(info, "User %d quit", user.uid);
        log_message(LOG_INFO, info);
        online = 0;
        break;
    }
  }

  return NULL;
}

void *send_msg()
{
  while (1) {
    while (messages->size == 0) {
      usleep(50);
    }

    pthread_mutex_lock(&message_mutex_lock);

    struct message_decode *_msg = dequeue_message(messages);
    struct message_encode to_be_sent = {SESSION_ONE, _msg->message_t};

    if (_msg->opt == SEND_PRIVATE) {
      node_t *cur = users->front;
      while (cur != NULL) {
        if (cur->user.uid == _msg->message_t.uid_to)
          break;
        cur = cur->next;
      }

      if (cur != NULL)
        write(cur->user.sockfd, (void *)&to_be_sent, sizeof(to_be_sent));
      else {
        // TODO Tell the client that user not found
      }
    }

    if (_msg->opt == SEND_PUBLIC) {
      node_t *cur = users->front;
      while (cur != NULL) {
        write(cur->user.sockfd, (void *)&to_be_sent, sizeof(to_be_sent));
        cur = cur->next;
      }
    }

    pthread_mutex_unlock(&message_mutex_lock);
  }

  return NULL;
}

void *broadcast_users()
{
  while (1) {
    char buff[MAX_BUFFER];

    pthread_mutex_lock(&users_mutex_lock);
    pthread_cond_wait(&user_change_cond, &users_mutex_lock);
    // log_message(LOG_DEBUG, "broadcast_users wake up");

    node_t *curr1 = users->front, *curr2 = users->front;
    time_t currtime;
    time(&currtime);

    while (curr1 != NULL) {
      curr2 = users->front;
      while (1) {
       bzero(buff, sizeof(buff));

       struct message_encode to_be_sent;
       to_be_sent.screem = SESSION_THREE;
       char userinfo[MAX_BUFFER];

       if (curr2 == users->front) {
         /* to_uid == from_uid == -1   represent for start of sending user list */
         to_be_sent.message_t.uid_from = to_be_sent.message_t.uid_to = -1;
       	 write(curr1->user.sockfd, (void *)&to_be_sent, sizeof(to_be_sent));
       }

       if (curr2 != NULL) {
	       sprintf(userinfo, "[%2d] %s", curr2->user.uid, curr2->user.user_name);
         /* to_uid == from_uid == -2   represent for sending user list */
         to_be_sent.message_t.uid_from = to_be_sent.message_t.uid_to = -2;
         strcpy(to_be_sent.message_t.content, userinfo);
       	 write(curr1->user.sockfd, (void *)&to_be_sent, sizeof(to_be_sent));
       } else {
         /* to_uid == from_uid == -3   represent for ending of sending user list */
         to_be_sent.message_t.uid_from = to_be_sent.message_t.uid_to = -3;
         write(curr1->user.sockfd, (void *)&to_be_sent, sizeof(to_be_sent));
         break;
       }

       curr2 = curr2->next;
      }
      curr1 = curr1->next;
    }

    pthread_mutex_unlock(&users_mutex_lock);
  }

  return NULL;
}

/**********************************
 * Functions manipulate the queue
 *********************************/

/* enqueue - add user to the user list */
void enqueue(queue_t *q, user_t user)
{
  node_t *new_node = (node_t *)malloc(sizeof(node_t));
  new_node->user = user;
  new_node->next = NULL;

  if (q->rear == NULL) {
    q->front = new_node;
    q->rear = new_node;
  } else {
    q->rear->next = new_node;
    q->rear = new_node;
  }

  q->size++;
  pthread_cond_signal(&user_change_cond);
}

/* dequeue - delete user from the user list */
void dequeue(queue_t *q, int uid)
{
  node_t *prev = NULL;
  node_t *curr = q->front;

  while (curr != NULL) {
    if (curr->user.uid == uid) {
      if (prev == NULL) q->front = curr->next;
      else prev->next = curr->next;

      if (curr == q->rear)
        q->rear = prev;

      close(curr->user.sockfd);
      free(curr);
      q->size--;
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  pthread_cond_signal(&user_change_cond);
}

/* enqueue_message - insert message to the message list */
void enqueue_message(message_queue_t *q, struct message_decode *message)
{
  if (q->size == MAX_MSG_NUM) {
    // log_message(LOG_DEBUG, "Too much messages");
    return;
  }

  q->messages[q->front] = message;

  q->rear = (q->rear + 1) % MAX_MSG_NUM;
  q->size++;
}

/* dequeue_message - pop message from the message list */
struct message_decode *dequeue_message(message_queue_t *q)
{
  if (q->size == 0) {
    return NULL;
  }

  struct message_decode *message = q->messages[q->front];
  q->front = (q->front + 1) % MAX_MSG_NUM;
  q->size--;
  return message;
}
