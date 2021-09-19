#define _FILE_OFFSET_BITS 64

#include "include/colors.h"
#include "include/common.h"
#include "include/getopts.h"
#include "include/onlinesearch.h"
#include "include/options.h"
#include "include/releaselists.h"
#include "include/sfo.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include "include/dirent.h"
#define DIR_SEPARATOR '\\'
#else
#include <dirent.h>
#include <signal.h>
#include <stdio_ext.h> // For __fpurge(); only on GNU systems
#include "include/terminal.h"
#define DIR_SEPARATOR '/'
#endif

char *illegal_characters = "\"*/:<>?\\|"; // https://www.ntfs.com/exfat-filename-dentry.htm

// Default settings
char format_string[MAX_FORMAT_STRING_LEN] =
  "%title% [%type%] [%title_id%] [%release_group%] [%release%] [%backport%]";
char placeholder_char = '_';
struct custom_category custom_category = {"Game", "Update %app_ver%", "DLC", "App", NULL};

int first_run;

// Removes unused curly braces expressions; returns 0 on success
int curlycrunch(char *string, int position) {
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
char *strreplace(char *string, char *search, char *replace) {
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

// Returns 1 if char c is illegal on an exFAT file system
int is_in_set(char c, char *set) {
  for (int i = 0; i < strlen(set); i++) {
    if (c == set[i]) {
      return 1;
    }
  }
  return 0;
}

// Replaces or hides (skips) illegal characters;
// returns number of non-printable characters
int replace_illegal_characters(char *string) {
  char buffer[strlen(string)];
  char *p = buffer;
  int count = 0;

  for (int i = 0; i < strlen(string); i++) {
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
      count++;
    }
  }
  *p = '\0';
  strcpy(string, buffer);

  return count;
}

// Converts a string to mixed-case style; returns number of changed characters
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

void rename_file(char *filename, char *new_basename, char *path) {
  FILE *file;
  char new_filename[MAX_FILENAME_LEN];

  strcpy(new_filename, path);
  strcat(new_filename, new_basename);

  file = fopen(new_filename, "rb");

  // File already exists
  if (file != NULL) {
    fclose(file);
    // Use temporary file to force-rename on (case-insensitive) exFAT
    if (lower_strcmp(filename, new_filename) == 0) {
      char temp[MAX_FILENAME_LEN];
      strcpy(temp, new_filename);
      strcat(temp, ".pkgrename");
      if (rename(filename, temp)) goto error;
      if (rename(temp, new_filename)) goto error;
    } else {
      fprintf(stderr, "File already exists: \"%s\".\n", new_filename);
      exit(1);
    }
  // File does not exist yet
  } else {
    if (rename(filename, new_filename)) goto error;
  }

  return;

  error:
  fprintf(stderr, "Could not rename file %s.\n", filename);
  exit(1);
}

void pkgrename(char *filename) {
  if (first_run) {
    printf("\n");
  } else {
    first_run = 1;
  }

  char new_basename[MAX_FILENAME_LEN]; // Used to build the new PKG file's name
  char *basename; // "filename" without path
  char lowercase_basename[MAX_FILENAME_LEN];
  char *path; // "filename" without file

  // Internal pattern variables
  char *app_ver = NULL;
  char *backport = NULL;
  char *category = NULL;
  char *content_id = NULL;
  char firmware[9] = "";
  char *release_group = NULL;
  char *release = NULL;
  char sdk[6] = "";
  char title[MAX_TITLE_LEN] = "";
  char *title_backup = NULL;
  char *title_id = NULL;
  char *type = NULL;
  char *version = NULL;

  int paramc;
  struct sfo_parameter *params;

  int special_char_count = 0;

  // Define the file's basename
  basename = strrchr(filename, DIR_SEPARATOR);
  if (basename == NULL) {
    basename = filename;
  } else {
    basename++;
  }

  // Define the file's path
  int path_len = strlen(filename) - strlen(basename);
  path = malloc(path_len + 1);
  strncpy(path, filename, path_len);
  path[path_len] = '\0';

  // Create a lowercase copy of "basename"
  strcpy(lowercase_basename, basename);
  for (int i = 0; i < strlen(lowercase_basename); i++) {
    lowercase_basename[i] = tolower(lowercase_basename[i]);
  }

  // Print current basename
  printf("   \"%s\"\n", basename);

  // Load SFO parameters into variables
  params = sfo_read(&paramc, filename);
  if (params == NULL) {
    switch (paramc) {
      case 0:
        fprintf(stderr, "Error while opening file \"%s\".\n", filename);
        exit(1);
      case 1:
        fprintf(stderr, "Param.sfo magic number not found in file \"%s\".\n",
          filename);
        break;
    }
    return;
  }
  for (int i = 0; i < paramc; i++) {
    if (strcmp(params[i].name, "APP_VER") == 0) {
      if (option_leading_zeros == 1)
        app_ver = params[i].string;
      else if (params[i].string[0] == '0')
        app_ver = &params[i].string[1];
    } else if (strcmp(params[i].name, "CATEGORY") == 0) {
      category = params[i].string;
      if (strcmp(params[i].string, "gd") == 0) {
        type = custom_category.game;
      } else if (strstr(params[i].string, "gp") != NULL) {
        type = custom_category.patch;
      } else if (strcmp(params[i].string, "ac") == 0) {
        type = custom_category.dlc;
      } else if (params[i].string[0] == 'g' && params[i].string[1] == 'd') {
        type = custom_category.app;
      } else {
        type = custom_category.other;
      }
    } else if (strcmp(params[i].name, "CONTENT_ID") == 0) {
      content_id = params[i].string;
    } else if (strcmp(params[i].name, "PUBTOOLINFO") == 0) {
      char *p = strstr(params[i].string, "sdk_ver=");
      if (p != NULL) {
        p += 8;
        memcpy(sdk, p, 4);
      }
      if (sdk[0] == '0' && option_leading_zeros == 0) {
        sdk[0] = sdk[1];
        sdk[1] = '.';
        sdk[4] = '\0';
      } else {
        sdk[4] = sdk[3];
        sdk[3] = sdk[2];
        sdk[2] = '.';
      }
    } else if (strcmp(params[i].name, "SYSTEM_VER") == 0) {
      sprintf(firmware, "%08x", params[i].integer);
      if (firmware[0] == '0' && option_leading_zeros == 0) {
        firmware[0] = firmware[1];
        firmware[1] = '.';
        firmware[4] = '\0';
      } else {
        firmware[5] = '\0';
        firmware[4] = firmware[3];
        firmware[3] = firmware[2];
        firmware[2] = '.';
      }
    } else if (strcmp(params[i].name, "TITLE") == 0) {
      title_backup = params[i].string;
      strncpy(title, params[i].string, MAX_TITLE_LEN);
      title[MAX_TITLE_LEN - 1] = '\0';
    } else if (strcmp(params[i].name, "TITLE_ID") == 0) {
      title_id = params[i].string;
    } else if (strcmp(params[i].name, "VERSION") == 0) {
      if (option_leading_zeros == 1)
        version = params[i].string;
      else if (params[i].string[0] == '0')
        version = &params[i].string[1];
    }
  }

  // Detect backport
  if (strstr(lowercase_basename, "backport") != NULL ||
      strstr(lowercase_basename, "bp") != NULL) {
    backport = "Backport";
  }

  // Detect releases
  release_group = get_release_group(lowercase_basename);
  release = get_uploader(lowercase_basename);

  // Get file size in GB/GiB
  ;
  int temp = 0;
  long int temp2 = 0;
  FILE *file = fopen(filename, "rb");
  fseek(file, 0, SEEK_END);
  temp = ftello(file) / 1000000000;
  temp2 = ftello(file) % 1000000000;
  fclose(file);
  char size[12];
  snprintf(size, sizeof size, "%d.%ld GB", temp, temp2 / 10000000);
  // TODO: GiB
  // TODO: Make calculations more efficient

  // Option "online"
  if (option_online == 1) {
    search_online(content_id, title, 0);
  }

  // Option "mixed-case"
  if (option_mixed_case == 1) {
    mixed_case(title);
  }

  // User input loop
  char c;
  while(1) {
    // Build new file name:
    // ========================================================================

    // Replace pattern variables
    strcpy(new_basename, format_string);
    // %type% first, as it may contain other pattern variables
    strreplace(new_basename, "%type%", type);
    strreplace(new_basename, "%app_ver%", app_ver);
    strreplace(new_basename, "%backport%", backport);
    strreplace(new_basename, "%category%", category);
    strreplace(new_basename, "%content_id%", content_id);
    strreplace(new_basename, "%firmware%", firmware);
    strreplace(new_basename, "%release_group%", release_group);
    strreplace(new_basename, "%release%", release);
    strreplace(new_basename, "%sdk%", sdk);
    strreplace(new_basename, "%size%", size);
    strreplace(new_basename, "%title%", title);
    strreplace(new_basename, "%title_id%", title_id);
    strreplace(new_basename, "%version%", version);

    // Remove empty brackets and parentheses, and curly braces
    while (strreplace(new_basename, "[]", "") != NULL)
      ;
    while (strreplace(new_basename, "()", "") != NULL)
      ;
    while (strreplace(new_basename, "{", "") != NULL)
      ;
    while (strreplace(new_basename, "}", "") != NULL)
      ;

    // Replace illegal characters
    special_char_count = replace_illegal_characters(new_basename);

    // Replace misused special characters
    strreplace(new_basename, "＆", "&");
    strreplace(new_basename, "’", "'");
    strreplace(new_basename, " ", " "); // Irregular whitespace
    strreplace(new_basename, "Ⅲ", "III");

    // Replace potentially annoying special characters
    strreplace(new_basename, "™", NULL);
    strreplace(new_basename, "®", NULL);
    strreplace(new_basename, "–", "-");

    // Remove any number of repeated spaces
    while (strreplace(new_basename, "  ", " ") != NULL)
      ;

    // Remove leading whitespace
    char *p;
    p = new_basename;
    while (isspace(p[0])) p++;
    memmove(new_basename, p, MAX_FILENAME_LEN);

    // Remove trailing whitespace
    p = new_basename + strlen(new_basename) - 1;
    while (isspace(p[0])) p[0] = '\0';

    // Option --underscores: replace all whitespace with underscores
    if (option_underscores == 1) {
      p = new_basename;
      while (*p != '\0') {
        if (isspace(*p)) *p = '_';
        p++;
      }
    }

    strcat(new_basename, ".pkg");
    // ========================================================================

    // Print new basename
    printf("=> \"%s\"", new_basename);
    if (option_verbose && special_char_count) {
      printf(BRIGHT_PURPLE " (%d)" RESET, special_char_count);
    }
    printf("\n");

    // Exit if new basename too long
    if (strlen(new_basename) > MAX_FILENAME_LEN - 1) {
      fprintf(stderr, "New filename too long (%ld/%d) characters).\n",
        strlen(new_basename) + 4, MAX_FILENAME_LEN - 1);
      exit(1);
    }

    // Option -n: don't do anything else
    if (option_no_to_all == 1) goto exit;

    // Quit if already renamed
    if (option_force == 0 && strcmp(basename, new_basename) == 0) {
      printf("Nothing to do.\n");
      goto exit;
    }

    // Rename now if option_yes_to_all enabled
    if (option_yes_to_all == 1) {
      rename_file(filename, new_basename, path);
      goto exit;
    }

    // Read user input
    printf("Rename? [Y]es [N]o [A]ll [E]dit [M]ix [O]nline [R]eset [C]hars [S]FO [Q]uit: ");
    do {
      #if defined(_WIN32) || defined(_WIN64)
      c = getch();
      #else
      __fpurge(stdin); // TODO: On non-Windows, non-GNU systems:
                       // Disable capital letter parsing instead
      c = getchar();
      #endif
    } while (strchr("ynaemorcsq", c) == NULL);
    printf("%c\n", c);

    // Evaluate user input
    switch (c) {
      case 'y': // [Y]es: rename the file
        rename_file(filename, new_basename, path);
        goto exit;
      case 'n': // [No]: skip file
        goto exit;
      case 'a': // [A]ll: rename files automatically
        option_yes_to_all = 1;
        rename_file(filename, new_basename, path);
        goto exit;
      case 'e': // [E]dit: let user manually enter a new title
        // TODO: scan_string() for Windows
        printf("\nEnter new title: ");
        #if ! defined (_WIN32) && ! defined (_WIN64)
        scan_string(title, MAX_FILENAME_LEN, title);
        printf("\n\n");
        #else
        fgets(title, MAX_FILENAME_LEN, stdin);
        printf("\n");
        #endif
        break;
      case 'm': // [M]ix: convert title to mixed-case letter format
        mixed_case(title);
        printf("\nConverted letter case to mixed-case style.\n\n");
        break;
      case 'o': // [O]nline: search the PlayStation store online for metadata
        printf("\n");
        search_online(content_id, title, 1);
        printf("\n");
        break;
      case 'r': // [R]eset: undo all changes
        strcpy(title, title_backup);
        printf("\nTitle has been reset to \"%s\".\n\n", title_backup);
        break;
      case 'c': // [C]hars: reveal all non-printable characters
        printf("\nOriginal: \"%s\"\nRevealed: \"", title_backup);
        int count = 0;
        for (int i = 0; i < strlen(title_backup); i++) {
          if (isprint(title_backup[i])) {
            printf("%c", title_backup[i]);
          } else {
            count++;
            printf(BRIGHT_YELLOW "#%d" RESET,
              title_backup[i]);
          }
        }
        printf("\"\n%d special character%c found.\n\n", count,
          count == 1 ? 0 : 's');
        break;
      case 's': // [S]FO: print param.sfo information
        printf("\n");
        print_sfo(filename);
        printf("\n");
        break;
      case 'q': // [Q]uit: exit the program
        exit(0);
        break;
    }
  }

  exit:
  free(params);
  free(path);
}

// Companion function for qsort in pkgrename_dir()
int qsort_compare_strings(const void *p, const void *q) {
  return strcmp(*(const char **)p, *(const char **)q);
}

// Finds all .pkg files in a directory and runs pkgrename() on them
void parse_directory(char *directory_name) {
  DIR *dir;
  struct dirent *directory_entry;
  char *directory_names[1000], *filenames[1000];
  int directory_count = 0, file_count = 0;

  // Remove a possibly trailing directory separator
  {
    int len = strlen(directory_name);
    if (len > 1 && directory_name[len - 1] == DIR_SEPARATOR) {
      directory_name[len - 1] = '\0';
    }
  }

  dir = opendir(directory_name);
  if (dir == NULL) return;
  // Read all directory entries to put them in lists
  while ((directory_entry = readdir(dir)) != NULL) {
    // Entry is directory
    if (directory_entry->d_type == DT_DIR) {
      // Save name in the directory list
      if (option_recursive == 1
        && strcmp(directory_entry->d_name, "..") != 0 // Exclude system directories
        && strcmp(directory_entry->d_name, ".") != 0)
      {
        directory_names[directory_count] = malloc(strlen(directory_name) + 1 + strlen(directory_entry->d_name) + 1);
        sprintf(directory_names[directory_count], "%s%c%s", directory_name, DIR_SEPARATOR, directory_entry->d_name);
        directory_count++;
      }
    // Entry is .pkg file
    } else {
      char *file_extension = strrchr(directory_entry->d_name, '.');
      if (file_extension != NULL && strcasecmp(file_extension, ".pkg") == 0) {
        // One-time message when entering a new directory that has .pkg files
        if (option_recursive == 1 && file_count == 0) {
          printf(GRAY "\nEntering directory \"%s\"\n\n" RESET, directory_name);
        }
        // Save name in the file list
        filenames[file_count] = malloc(strlen(directory_name) + 1 + strlen(directory_entry->d_name) + 1);
        sprintf(filenames[file_count], "%s%c%s", directory_name, DIR_SEPARATOR, directory_entry->d_name);
        file_count++;
      }
    }
  }

  // Sort the final lists
  qsort(directory_names, directory_count, sizeof(char *), qsort_compare_strings);
  qsort(filenames, file_count, sizeof(char *), qsort_compare_strings);

  // Files first
  for (int i = 0; i < file_count; i++) {
    pkgrename(filenames[i]);
    free(filenames[i]);
  }

  // Then directories
  if (option_recursive == 1) {
    for (int i = 0; i < directory_count; i++) {
      parse_directory(directory_names[i]);
      free(directory_names[i]);
    }
  }

  closedir(dir);
}

int main(int argc, char *argv[]) {
  // Set terminal to noncanonical input processing mode
  #if ! defined(_WIN32) && ! defined(_WIN64)
  initialize_terminal();
  raw_terminal();
  #endif

  parse_options(argc, argv);

  if (noptc == 0) { // Use current directory
    parse_directory(".");
  } else { // Find PKGs and run pkgrename() on them
    DIR *dir;
    for (int i = 0; i < noptc; i++) {
      // Directory
      if ((dir = opendir(nopts[i])) != NULL) {
        closedir(dir);
        parse_directory(nopts[i]);
      // File
      } else {
        pkgrename(nopts[i]);
      }
    }
  }
  exit(0);
}
