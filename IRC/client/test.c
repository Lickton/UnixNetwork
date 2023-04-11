#include <ncurses.h>

int main()
{
    // Initialize ncurses
    initscr();

    // Create two windows
    int height, width;
    getmaxyx(stdscr, height, width);
    WINDOW *topwin = newwin(height/2-1, width, 0, 0);
    WINDOW *botwin = newwin(height/2, width, height/2, 0);

    // Draw the message in the top window
    wprintw(topwin, "Welcome to my program!");

    // Draw the options in the bottom window
    wprintw(botwin, "Select an option:\n");
    wprintw(botwin, "-> Option 1\n");
    wprintw(botwin, "   Option 2\n");
    wprintw(botwin, "   Option 3\n");

    // Draw a horizontal line between the two windows
    for (int i = 0; i < width; i++)
    {
        mvaddch(height/2-1, i, ACS_HLINE);
    }

    // Refresh the windows
    wrefresh(topwin);
    wrefresh(botwin);

    // Allow the user to select an option
    int ch;
    int option = 1;
    while ((ch = getch()) != '\n')
    {
        // Move the cursor based on arrow key input
        switch (ch)
        {
            case KEY_UP:
                if (option > 1) option--;
                break;
            case KEY_DOWN:
                if (option < 3) option++;
                break;
        }

        // Highlight the selected option
        wclear(botwin);
        wprintw(botwin, "Select an option:\n");
        if (option == 1) wprintw(botwin, "-> Option 1\n");
        else wprintw(botwin, "   Option 1\n");
        if (option == 2) wprintw(botwin, "-> Option 2\n");
        else wprintw(botwin, "   Option 2\n");
        if (option == 3) wprintw(botwin, "-> Option 3\n");
        else wprintw(botwin, "   Option 3\n");
        wrefresh(botwin);
    }

    // Clean up and exit
    delwin(topwin);
    delwin(botwin);
    endwin();
    return 0;
}

