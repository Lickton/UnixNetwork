#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 55312
#define BUFFER_SIZE 1024

int main(int argc, char **argv)
{
  /* initialze socket */
  int server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sockfd == -1) {
    perror("Error creating server socker");
    return EXIT_FAILURE;
  }

  /* bind socket */
  struct sockaddr_in servAddr;
  bzero(&servAddr, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(PORT);

  int bind_result = bind(server_sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
  if (bind_result == -1) {
    perror("Error binding server socket to address");
    return EXIT_FAILURE;
  }

  /* listen socket */
  int listen_result = listen(server_sockfd, 5);
  if (listen_result == -1) {
    perror("Error listening on server socket");
    return EXIT_FAILURE;
  }


  while (1) {
    printf("waiting for connection\n");

    /* accept request */
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_sockfd == -1) {
      perror("Error accepting client connection");
      continue;
    }

    printf("Acceptd client connection\n");

    /* fork to precess request */
    pid_t pid = fork();
    if (pid == -1) {
      perror("Error forking process to handle request");
      close(client_sockfd);
      continue;
    } else if (pid == 0) {
      /* child process */
      close(server_sockfd);

      /*redirect output to STDIN and STDOUT */
      dup2(client_sockfd, STDIN_FILENO);
      dup2(client_sockfd, STDOUT_FILENO);

      /* execute the program requested */
      char *args[] = {"./messageHandler", NULL};
      execl(args[0], (const char*)args, NULL);

      /* execl fails print the error message and exit */
      perror("Error executing handle_request_program");
      exit(EXIT_FAILURE);
    } else {
      /* father process */
      close(client_sockfd);
    }
  }

  /* control never reach here */
  close(server_sockfd);
  return EXIT_SUCCESS;
}
