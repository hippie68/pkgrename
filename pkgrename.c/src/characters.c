#include "../include/characters.h"
#include "../include/options.h"

#include <ctype.h>
#include <string.h>

char *illegal_characters = "\"*/:<>?\\|"; // https://www.ntfs.com/exfat-filename-dentry.htm
char placeholder_char = '_';

// Returns 1 if char c is illegal on an exFAT file system
int is_in_set(char c, char *set) {
  for (size_t i = 0; i < strlen(set); i++) {
    if (c == set[i]) {
      return 1;
    }
  }
  return 0;
}

// Returns number of special characters in a string
int count_spec_chars(char *string) {
  int count = 0;

  for (size_t i = 0; i < strlen(string); i++) {
    if (!isprint(string[i])) count++;
  }

  return count;
}

// Replaces or hides (skips) illegal characters;
void replace_illegal_characters(char *string) {
  char buffer[strlen(string)];
  char *p = buffer;

  for (size_t i = 0; i < strlen(string); i++) {
    // Printable character
    if (isprint(string[i])) {
      if (is_in_set(string[i], illegal_characters)) {
        // Replace illegal character
        if (option_no_placeholder) {
          *(p++) = ' '; // Replace with space
        } else {
          *(p++) = placeholder_char; // Replace with placeholder
        }
      } else {
        *(p++) = string[i];
      }
    // Non-printable character
    } else { // TODO: Skip user-provided non-printable characters
      *(p++) = string[i];
    }
  }

  *p = '\0';
  strcpy(string, buffer);
}

