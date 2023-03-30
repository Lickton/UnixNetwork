#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 55312

int main()
{
  char buf[1024];
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
  
  if (connect(socketClient, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
    printf("connect falied\n");
    return -1;
  }

  printf("connect to the server\n");

  close(socketClient);
  return 0;
}
