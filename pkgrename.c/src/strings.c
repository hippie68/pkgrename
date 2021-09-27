#include "../include/characters.h"
#include "../include/common.h"
#include "../include/strings.h"

#include <ctype.h>
#include <string.h>

// Removes unused curly braces expressions; returns 0 on success
static int curlycrunch(char *string, int position) {
  char temp[MAX_FILENAME_LEN] = "";

  // Search left
  for (int i = position; i >= 0; i--) {
    if (string[i] == '{') {
      strncpy(temp, string, i);
      //printf("%s left : %s\n", __func__, temp); // DEBUG
      break;
    }
    if (string[i] == '}') {
      return 1;
    }
  }

  // Search right
  for (int i = position; i < strlen(string); i++) {
    if (string[i] == '}') {
      strcat(temp, &string[i + 1]);
      //printf("%s right: %s\n", __func__, temp); // DEBUG
      break;
    }
    if (string[i] == '{' || i == strlen(string) - 1) {
      return 1;
    }
  }

  strcpy(string, temp);
  return 0;
}

// Replaces first occurence of "search" in "string" (an array of char), e.g.:
// strreplace(temp, "%title%", title)
// "string" must be of length MAX_FILENAME_LEN
inline char *strreplace(char *string, char *search, char *replace) {
  char *p;
  char temp[MAX_FILENAME_LEN] = "";
  int position; // Position of first character of "search" in "format_string"
  p = strstr(string, search);
  if (p != NULL) {
    if (replace == NULL) replace = "";
    position = p - string;

    //printf("Search string: %s\n", search); // DEBUG
    //printf("Replace string: %s\n", replace); // DEBUG

    // If replace string is an empty pattern variable, remove associated
    // curly braces expression
    if (search[0] == '%' && strcmp(replace, "") == 0 &&
      curlycrunch(string, position) == 0) return NULL;

    strncpy(temp, string, position);
    //printf("Current string (step 1): \"%s\"\n", temp); // DEBUG
    strcat(temp, replace);
    //printf("Current string (step 2): \"%s\"\n", temp);  // DEBUG
    strcat(temp, string + position + strlen(search));
    //printf("Current string (step 3): \"%s\"\n\n", temp);  //DEBUG
    strcpy(string, temp);
  }
  return p;
}

// Converts a string (title, to be specific) to mixed-case style
void mixed_case(char *title) {
  int title_is_prepared = 0;
  int len = strlen(title);

  // Apply mixed-case style
  title[0] = toupper(title[0]);
  for (int i = 1; i < len; i++) {
    if (isspace(title[i - 1]) || is_in_set(title[i - 1], ":-;~_1234567890")) {
      title[i] = toupper(title[i]);
    } else {
      title[i] = tolower(title[i]);
    }
  }

  // Prepare title for special case rules comparisons by adding a space
  if (len < MAX_TITLE_LEN) {
    title[len] = ' ';
    title[len + 1] = '\0';
    title_is_prepared = 1;
  }

  // Special case rules
  strreplace(title, "Dc", "DC");
  strreplace(title, "Dlc", "DLC");
  strreplace(title, "Dx", "DX");
  strreplace(title, "Hd", "HD");
  strreplace(title, "hd", "HD");
  strreplace(title, "Vs", "vs");
  strreplace(title, "Vr ", "VR ");
  strreplace(title, "Iii", "III");
  strreplace(title, "Ii", "II");
  strreplace(title, " Iv:", " IV:");
  strreplace(title, " Iv ", " IV ");
  strreplace(title, "Viii", "VIII");
  strreplace(title, "Vii", "VII");
  strreplace(title, " Vi ", " VI ");
  strreplace(title, " Vi:", " VI:");
  strreplace(title, "Ix", "IX");
  strreplace(title, "Xiii", "XIII");
  strreplace(title, "Xii", "XII");
  strreplace(title, "Xiv", "XIV");
  strreplace(title, "Xi", "XI");
  strreplace(title, "Xvi", "XVI");
  strreplace(title, "Xv", "XV");
  strreplace(title, "Playstation", "PlayStation");

  // Game-specific rules
  strreplace(title, "Crosscode", "CrossCode");
  strreplace(title, "Littlebigplanet", "LittleBigPlanet");
  strreplace(title, "Nier", "NieR");
  strreplace(title, "Ok K.o.", "OK K.O.");
  strreplace(title, "Pixeljunk", "PixelJunk");
  strreplace(title, "Singstar", "SingStar");
  strreplace(title, "Snk", "SNK");
  strreplace(title, "Wwe", "WWE");
  strreplace(title, "Xcom", "XCOM");

  // Undo title preparation
  if (title_is_prepared) {
    title[len] = '\0';
  }
}

int lower_strcmp(char *string1, char *string2) {
  if (strlen(string1) == strlen(string2)) {
    for (int i = 0; i < strlen(string1); i++) {
      if (tolower(string1[i]) != tolower(string2[i])) {
        return 1;
      }
    }
  } else {
    return 1;
  }

  return 0;
}
