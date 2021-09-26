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

// Converts a string to mixed-case style
void mixed_case(char *string) {
  string[0] = toupper(string[0]);
  for (int i = 1; i < strlen(string); i++) {
    if (isspace(string[i - 1]) || is_in_set(string[i - 1], ":-;~_1234567890")) {
      string[i] = toupper(string[i]);
    } else {
      string[i] = tolower(string[i]);
    }
  }

  // Special case rules
  strreplace(string, "Dc", "DC");
  strreplace(string, "Dlc", "DLC");
  strreplace(string, "Dx", "DX");
  strreplace(string, "Hd", "HD");
  strreplace(string, "hd", "HD");
  strreplace(string, "Vs", "vs");
  strreplace(string, "Vr ", "VR ");
  strreplace(string, "Vr\0", "VR\0");
  strreplace(string, "Iii", "III");
  strreplace(string, "Ii", "II");
  strreplace(string, " Iv", " IV");
  strreplace(string, " Iv:", " IV:");
  strreplace(string, " Iv\0", " IV\0");
  strreplace(string, "Viii", "VIII");
  strreplace(string, "Vii", "VII");
  strreplace(string, " Vi ", " VI ");
  strreplace(string, " Vi:", " VI:");
  strreplace(string, " Vi\0", " VI\0");
  strreplace(string, "Ix", "IX");
  strreplace(string, "Xiii", "XIII");
  strreplace(string, "Xii", "XII");
  strreplace(string, "Xiv", "XIV");
  strreplace(string, "Xi", "XI");
  strreplace(string, "Xvi", "XVI");
  strreplace(string, "Xv", "XV");
  strreplace(string, "Playstation", "PlayStation");

  // Game-specific rules
  strreplace(string, "Crosscode", "CrossCode");
  strreplace(string, "Littlebigplanet", "LittleBigPlanet");
  strreplace(string, "Nier", "NieR");
  strreplace(string, "Ok K.o.", "OK K.O.");
  strreplace(string, "Pixeljunk", "PixelJunk");
  strreplace(string, "Singstar", "SingStar");
  strreplace(string, "Snk", "SNK");
  strreplace(string, "Wwe", "WWE");
  strreplace(string, "Xcom", "XCOM");
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
