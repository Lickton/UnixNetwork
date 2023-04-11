#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <curses.h>

#include "../include/log.h"

/* Misc manifest constants */
#define MAX_BUFFER		1024				/* max buffer size */
#define PORT					55312				/* use this port to communicate */

/* Global variables */
WINDOW *top;											/* the top part of the terminal */
WINDOW *bot;											/* the bottom part of the terminal */

/* Function prototypes */
void init_GUI(void);
void cleanup_GUI(void);

int main()
{
  log_message(LOG_INFO, "Client starts");
  init_GUI();

  wprintw(top, "This is 1");
  wprintw(bot, "This is 2");
  wrefresh(top);
  wrefresh(bot);

  getch();

  // clean up and exit
  cleanup_GUI();
  return EXIT_SUCCESS;
}

/****************************
 * Routines about the curses
 ***************************/

/* init_GUI - initialize the windows */
void init_GUI(void)
{
  initscr();
  raw();
  int height, width;
  getmaxyx(stdscr, height, width);
  top = newwin(height/2, width, 0, 0);
  bot = newwin(height/2, width, height/2, 0);

  refresh();

  for (int i = 0; i < width; i++) {
    mvaddch(height/2-1, i, ACS_HLINE);
  }
}

/* cleanup_GUI - clean up the screen when quit */
void cleanup_GUI(void)
{
  delwin(top);
  delwin(bot);
  endwin();
}
