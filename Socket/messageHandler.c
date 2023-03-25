#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

int main(int argc, char **argv)
{
  char buff[1024];
  read(STDOUT_FILENO, buff, sizeof(buff));

  write(STDOUT_FILENO, buff, sizeof(buff));
  return 0;
}
