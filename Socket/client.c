#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 55312

int main(int argc, char** argv)
{
  char buf[1024];
  char msg[] = "Hello, I'am the client\n";
  printf("This is the client\n");
  
  // create the socket
  int socketClient = socket(AF_INET, SOCK_STREAM, 0);
  if (socketClient == -1) {
    printf("create socket failed\n");
    return -1;
  }

  struct sockaddr_in servAddr;
  bzero(&servAddr, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  // servAddr.sin_addr.s_addr = inet_addr("124.222.94.52");
  servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servAddr.sin_port = htons(PORT);
  
  int conn = connect(socketClient, (struct sockaddr*)&servAddr, sizeof(servAddr));
  if (conn < 0) {
    printf("connect falied\n");
    return -1;
  }

  printf("connect to the server\n");
  printf("sending message...\n");

  send(conn, msg, sizeof(msg), 0);
  read(socketClient, buf, sizeof(buf));

  printf("receiving message...\n");
  printf("%s\n", buf);

  close(socketClient);
  printf("socket is closed\n");
  return 0;
}
