#include "../include/common.h"
#include "../include/terminal.h"

#include <ctype.h>
#include <signal.h>

#ifdef _WIN32
static unsigned int win_orig_codepage;
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
static struct termios terminal;
static struct termios terminal_backup;
#endif

// For scan_string()
#ifdef _WIN32
#include <stdio.h>
#define getchar getkey
static HANDLE hStdin;
static DWORD fdwSaveOldMode;
static char WIN_KEY;
#endif

// Sets terminal to noncanonical input mode (= no need to press enter)
void raw_terminal() {
  #ifndef _WIN32
  terminal.c_lflag &= ~(ICANON);
  terminal.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
  #endif
}

// Resets terminal
void reset_terminal() {
  #ifndef _WIN32
  tcsetattr(STDIN_FILENO, TCSANOW, &terminal_backup);
  #endif
}

// Resets terminal automatically on exit
void atexit_reset_terminal() {
  #ifdef _WIN32
  SetConsoleOutputCP(win_orig_codepage);
  SetConsoleMode(hStdin, fdwSaveOldMode);
  #endif
  reset_terminal();
}

// Needs to be called before other functions
void initialize_terminal() {
  atexit(atexit_reset_terminal);

  // Windows
  #ifdef _WIN32
  win_orig_codepage = GetConsoleOutputCP(); // Store current codepage
  SetConsoleOutputCP(65001); // UTF-8 codepage

  hStdin = GetStdHandle(STD_INPUT_HANDLE);

  if (hStdin == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Invalid input handle.\n");
    exit(1);
  }

  if (! GetConsoleMode(hStdin, &fdwSaveOldMode) ) {
    fprintf(stderr, "Error while calling GetConsoleMode().\n");
    exit(1);
  }

  // Other OS
  #else
  tcgetattr(STDIN_FILENO, &terminal);
  terminal_backup = terminal;  // Store current terminal attributes
  signal(SIGINT, exit);
  #endif
}

#ifdef _WIN32
// Windows replacement for "getchar() in raw mode"
int getkey() {
  DWORD cNumRead;
  INPUT_RECORD irInBuf[128];

  // Repeat until key pressed
  while (ReadConsoleInput(hStdin, irInBuf, 128, &cNumRead)) {
    for (int i = 0; i < cNumRead; i++) {
      switch(irInBuf[i].EventType) {
        case KEY_EVENT:
          if (irInBuf[i].Event.KeyEvent.bKeyDown) {
            WIN_KEY = irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
            return irInBuf[i].Event.KeyEvent.wVirtualKeyCode;
          } else {
            WIN_KEY = 0;
          }
        }
    }
  }

  fprintf(stderr, "Error while calling ReadConsoleInput().\n");
  exit(1);
}
#endif

// Erase rest of the line
static void clear_line() {
  #ifdef _WIN32
  for (int i = 0; i < MAX_TAG_LEN; i++) printf(" ");
  for (int i = 0; i < MAX_TAG_LEN; i++) printf("\b");
  #else
  printf("\033[0J");
  #endif
}

// User input function with editing capabilities; for use with raw_terminal().
// Function f() is optional; its return value is used for auto-completion.
// Currently not working with Unicode characters.
void scan_string(char *string, int max_size, char *default_string, char *(*f)())
{
  #ifdef _WIN32
  #define KEY_BACKSPACE 8
  #define KEY_DELETE 46
  #define KEY_ENTER 13
  #define KEY_LEFT_ARROW 37
  #define KEY_RIGHT_ARROW 39
  #else
  #define KEY_BACKSPACE 127
  #define KEY_DELETE 126
  #define KEY_ENTER 10
  #define KEY_LEFT_ARROW 68
  #define KEY_RIGHT_ARROW 67
  #endif

  char key;
  char buffer[max_size * 2];
  char *autocomplete = NULL;

  strncpy(buffer, default_string, max_size);
  printf("%s", buffer);
  int i = strlen(buffer);

  while ((key = getchar()) != KEY_ENTER) { // Enter key
    //printf("Number: %d\n", key); // DEBUG
    switch (key) {
      #ifndef _WIN32
      case 27:
        while ((key = getchar()) == 27) // Ignore any follow-up escapes
          ;
        if (key == 91) { // ANSI escape sequence
          switch (getchar()) {
      #endif
            case KEY_LEFT_ARROW:
              if (f == NULL && i > 0) {
                i--;
                printf("\b"); // Move cursor left
              }
              break;
            case KEY_RIGHT_ARROW:
              if (i < strlen(buffer)) {
                #ifdef _WIN32
                printf("%c", buffer[i]);
                #else
                printf("\033[1C"); // Move cursor right
                #endif
                i++;
              }
              break;
      #ifndef _WIN32
          }
        } else goto Default; // Regular key
        break;
      #endif
      case KEY_DELETE: // Delete
        memmove(&buffer[i], &buffer[i + 1], max_size);
        printf("%s \b", &buffer[i]);
        for (int count = strlen(buffer); count > i; count --) {
          printf("\b");
        }
        break;
      case KEY_BACKSPACE: // Backspace
        if (i > 0) {
          printf("\b \b");
          memmove(&buffer[i - 1], &buffer[i], max_size);
          i--;
          printf("%s \b", &buffer[i]);
          for (int count = strlen(buffer); count > i; count --) {
            printf("\b");
          }
        }
        break;
      default: // Regular key
        Default:
        #ifdef _WIN32
        key = WIN_KEY;
        #endif
        if (isprint(key) && i < max_size - 1) {
          printf("%c", key);
          memmove(&buffer[i + 1], &buffer[i], max_size);
          buffer[i] = key;
          i++;
          printf("%s", &buffer[i]);
          for (int count = strlen(buffer); count > i; count --) {
            printf("\b");
          }
        }
    }

    // Call provided autocomplete function and print its result
    if (f != NULL) {
      clear_line();
      autocomplete = f(buffer);
      if (autocomplete != NULL) {
        printf("  [%s]", autocomplete);
        for (int i = 0; i < strlen(autocomplete) + 4; i++) {
          printf("\b");
        }
      }
    }
  }

  // Return result...
  if (f != NULL && autocomplete != NULL) { // ...of auto-completion
    clear_line();
    strncpy(string, autocomplete, max_size);
  } else {
    strncpy(string, buffer, max_size); // ..of regular input
  }

  printf("\n");
}

