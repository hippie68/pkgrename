#include "../include/terminal.h"

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
static struct termios terminal, terminal_backup;
#endif

// Resets terminal
void reset_terminal() {
  #ifdef _WIN32
  SetConsoleOutputCP(win_orig_codepage); // Restore stored codepage
  #else
  tcsetattr(STDIN_FILENO, TCSANOW, &terminal_backup);
  #endif
}

// Needs to be called before other functions
void initialize_terminal() {
  #ifdef _WIN32
  win_orig_codepage = GetConsoleOutputCP(); // Store current codepage
  SetConsoleOutputCP(65001); // UTF-8 codepage
  #else
  tcgetattr(STDIN_FILENO, &terminal);
  terminal_backup = terminal;
  signal(SIGINT, exit);
  #endif
  atexit(reset_terminal);
}

#ifndef _WIN32
// Sets terminal to noncanonical input mode (= no need to press enter)
void raw_terminal() {
  terminal.c_lflag &= ~(ICANON);
  terminal.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &terminal);
}

// For use with raw_terminal():
// Rudimentary user input function with editing capabilities; might have bugs
void scan_string(char *string, int max_size, char *default_string) {
  char key;
  char buffer[max_size * 2];

  strncpy(buffer, default_string, max_size);
  printf("%s", buffer);
  int i = strlen(buffer);

  while ((key = getchar()) != 10) { // Enter key
    //printf("Number: %d\n", key);
    switch (key) {
      case 9: // Tab; ignore
        break;
      case 27:
        while ((key = getchar()) == 27) // Ignore any follow-up escapes
          ;
        if (key == 91) { // Escape sequence
          switch (getchar()) {
            case 68: // Left arrow key
              if (i > 0) {
                i--;
                printf("\b"); // Move cursor left
              }
              break;
            case 67: // Right arrow key
              if (i < strlen(buffer)) {
                printf("\033[1C"); // Move cursor right
                i++;
              }
              break;
            /*
            default:
              printf("UNKNOWN SEQUENCE\n");
            */
          }
        } else goto Default; // Regular key
        break;
      case 126: // Delete
        memmove(&buffer[i], &buffer[i + 1], max_size);
        printf("%s \b", &buffer[i]);
        for (int count = strlen(buffer); count > i; count --) {
          printf("\b");
        }
        break;
      case 127: // Backspace
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
        if (i < max_size) {
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
  }
  strncpy(string, buffer, max_size);
  printf("\n");
}
#endif
