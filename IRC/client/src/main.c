#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ncurses.h>
#include <pthread.h>
#include <sys/select.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../include/log.h"

/* Misc manifest constants */
#define MAX_BUFFER 			1024
#define MAX_MSG_LEN 		256
#define MAX_MSG_NUM 		100
#define MAX_NAME_LEM 		20
#define MAX_USERS 			50
#define PORT 						55312
#define SERVER_IPV_4		"127.0.0.1"

/* Global variables */
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

struct message_decode {
  screen_num screem;
  struct message message_t;
};

struct message_encode {
  command_option opt;
  struct message message_t;
};

typedef struct user {
  int uid;
  char user_name[MAX_NAME_LEM];
  int sockfd;
} user_t;
user_t host;

WINDOW *win1, *win2, *win3;
const char prompt[] = "chqt-ggt>";
/* End Global variables */

/* Function prototypes */
char *get_name();
void *recv_message();

/* main - main function */
int main(int args, char *argv[])
{
  log_message(LOG_INFO, "Client starts");
  strcpy(host.user_name, get_name());

  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd < 0) {
    log_message(LOG_ERROR, "Create socket failed");
    return -1;
  }

  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family 			= AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(SERVER_IPV_4);
  server_addr.sin_port 				= htons(PORT);

  if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    log_message(LOG_ERROR, "Connect failed");
    return -1;
  }

  host.sockfd = client_fd;

  /* Initialize file descriptor set for select */
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(host.sockfd, &read_fds);
  FD_SET(STDIN_FILENO, &read_fds);
  int max_fd = host.sockfd;

  time_t currtime;
  time(&currtime);
  struct message_encode name_info;
  name_info.opt = REGISTE;
  strcpy(name_info.message_t.user_name, host.user_name);
  name_info.message_t.timestamp = currtime;

  write(client_fd, (void *)&name_info, sizeof(name_info));
  read(client_fd, (void *)&host.uid, sizeof(host.uid));

  log_message(LOG_INFO, "Registe successfully");

  initscr();
  cbreak();
  noecho();
  curs_set(0);

  /* Define color pairs */
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);

  
  refresh();// 核心代码

  win1 = newwin(LINES * 2/3, COLS * 2/3, 0, 0);
  win2 = newwin(LINES * 1/3, COLS * 2/3, LINES * 2/3, 0);
  win3 = newwin(LINES, COLS * 1/3, 0, COLS * 2/3);

  keypad(win2, TRUE);

  box(win1, 0, 0);
  box(win2, 0, 0);
  box(win3, 0, 0);

	mvwprintw(win1, 0, 2, " Chat Room ");
	mvwprintw(win2, 0, 2, " Input ");
	mvwprintw(win3, 0, 2, " User List ");

  wrefresh(win1);
  wrefresh(win2);
  wrefresh(win3);

  int online = 1;
  char str[MAX_MSG_LEN] = {0};
  int ch, len = 0;
  int win1_position = 1, win3_position = 1;

  struct message_encode sentence;
  sentence.opt = SEND_PUBLIC;
  sentence.message_t.uid_from = host.uid;
  sentence.message_t.uid_to = -5;

  while (online) {
    fd_set tmp_fds = read_fds;
    int activity = select(max_fd + 1, &tmp_fds, NULL, NULL, NULL);
    if (activity < 0) {
      log_message(LOG_ERROR, "Select error");
      return -1;
    }

    /* Check for activity from server */
    if (FD_ISSET(host.sockfd, &tmp_fds)) {
      log_message(LOG_DEBUG, "Message from server");

      struct message_decode msg;
      read(host.sockfd, (void *)&msg, sizeof(msg));

      /* Message from other users */
      if (msg.screem == SESSION_ONE) {
        log_message(LOG_DEBUG, "Message from Chat room");
        wmove(win1, 1, 1);
        /* private message printed in red */
        if (msg.message_t.uid_to > 0)
          attron(COLOR_PAIR(1));
        mvwprintw(win1, win1_position, 1, "%s", msg.message_t.content);
        if (msg.message_t.uid_to > 0)
          attroff(COLOR_PAIR(1));

        win1_position += 2;
        wrefresh(win1);
      /* Message sent to update the user list */
      } else if (msg.screem == SESSION_THREE) {
        log_message(LOG_DEBUG, "Message of user list");
        /* to_uid == from_uid == -1 clear up the screen */
        if (msg.message_t.uid_to == -1) {
          wclear(win3);
          box(win3, 0, 0);
          mvwprintw(win3, 0, 2, " User List ");
          wrefresh(win3);
          win3_position = 1;
        /* to_uid == from_uid == -2 add info to the user list */
        } else if (msg.message_t.uid_to == -2) {
          log_message(LOG_DEBUG, msg.message_t.content);
          mvwprintw(win3, win3_position, 1, "%s", msg.message_t.content);
          wrefresh(win3);
          win3_position++;
        /* to_uid == from_uid == -3 end of update */
        } else if (msg.message_t.uid_to == -3) {
          /* no need to do anything */
        }

      }
    }

    /* check for activity from stdin */
    if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
      // Read ch from stdin
      // Print to the win2
      // Press '\n'
      // 		1. send message
      // 		2. quit when press <ESC>
      //    3. switch to private or public when press <F12> TODO
      // 							when private set read number flag true and read uid
      wmove(win2, 1, 1);
      ch = wgetch(win2);
      if (ch == '\n' && len > 0) {

        char buff1[50], buff2[MAX_BUFFER];
        struct tm *loctime;
        time_t timestamp; time(&timestamp);
        loctime = localtime(&timestamp);
        strftime(buff1, sizeof(buff1), "%H:%M:%S", loctime);

        sentence.opt= SEND_PUBLIC;
        sprintf(buff2, "[%s][%d][%s]>%s", buff1, host.uid, host.user_name, str);
        strcpy(sentence.message_t.content, buff2);
        write(host.sockfd, (void *)&sentence, sizeof(sentence));

        strcpy(sentence.message_t.content, "\0");

        len = 0;
        bzero(str, sizeof(str));
        wclear(win2);
        box(win2, 0, 0);
        mvwprintw(win2, 0, 2, " Input ");
      } else if (ch == KEY_BACKSPACE && len > 0) {
        log_message(LOG_DEBUG, "Press BACKSPACE");
        str[--len] = '\0';
      } else if ((isalnum(ch) || ispunct(ch) || ch == ' ') && len < MAX_BUFFER) {

        str[len] = ch;
        str[++len] = '\0';
      } else if (ch == 27) {

        log_message(LOG_DEBUG, "Press ESC");
        online = 0;

        sentence.opt = QUIT;
        write(host.sockfd, (void *)&sentence, sizeof(sentence));
      }

      mvwprintw(win2, 1, 1, "%-30s", str);
      refresh();
      wrefresh(win2);
    }
  }

  delwin(win1);
  delwin(win2);
  delwin(win3);

  endwin();
  return EXIT_SUCCESS;
}

/***************************************
 * Ncurses Routines - Drawing Windows
 **************************************/

/* get_name - draw a hello screen and enter a name */
char *get_name()
{
  initscr();
  cbreak();
  noecho();
  curs_set(0);

  int win_width = 30;
  int win_height = 3;
  int win_x = (COLS - win_width) / 2;
  int win_y = (LINES - win_height) / 2;
  WINDOW* win = newwin(win_height, win_width, win_y, win_x);
  keypad(win, TRUE);

  box(win, 0, 0);

  mvwprintw(win, 0, 3, " Please enter your name ");

  mvwprintw(win, 1, win_width - 6, "00/15");

  char str[MAX_NAME_LEM] = {0};
  int ch;
  int len = 0;
  while (1)
  {
    ch = wgetch(win);
    if (ch == '\n' && len > 0) {
      break;
    }

    if (len >= MAX_NAME_LEM - 5 && ch != KEY_BACKSPACE)
      continue;
    
    if (ch == KEY_BACKSPACE && len > 0) {
      str[--len] = '\0';
    } else if (isalnum(ch) && len < MAX_NAME_LEM - 5) {
      str[len] = ch;
      str[++len] = '\0';
    }

    mvwprintw(win, 1, win_width - 6, "%02d/15", len);

    mvwprintw(win, 1, 2, "%-15s", str);
    wrefresh(win);
  }

  char *name = (char *)malloc(sizeof(char) * MAX_NAME_LEM);
  strcpy(name, str);

  delwin(win);
  endwin();

  char info[MAX_BUFFER];
  sprintf(info, "User name:%s", name);
  log_message(LOG_INFO, info);

  return name;
}
