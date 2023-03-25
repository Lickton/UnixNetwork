#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define PORT 55312

int main()
{
  char buf[1024];
  printf("This is the server\n");
  
  // create the socket
  int socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socketServer == -1) {
    printf("create socket failed\n");
    return -1;
  }

  printf("the socket is created\n");

  // bind the socket
  struct sockaddr_in servAddr;
  bzero(&servAddr, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(PORT);
  
  if (bind(socketServer, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    printf("bind falied, errno:%d\n", errno);
    return -1;
  }

  printf("the address is bound\n");

  // convert the state into listen
  if (listen(socketServer, 5) == -1) {
    printf("listen failed\n");
    return -1;
  }

  printf("listen state\n");

  // wait for connection
  struct sockaddr_in cliAddr;
  socklen_t len;

  while (1) {
    printf("waiting for connection\n");
    int conn = accept(socketServer, (struct sockaddr*)(&cliAddr), &len);

    if (conn == -1) {
      printf("accept failed\n");
      return -1;
    }

    printf("a client is connect\n\n");
  }

  close(socketServer);
  printf("the socket is closed\n");
  return 0;
}
