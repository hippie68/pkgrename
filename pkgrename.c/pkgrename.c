#define _FILE_OFFSET_BITS 64

#include "include/characters.h"
#include "include/colors.h"
#include "include/common.h"
#include "include/dirparse.h"
#include "include/getopts.h"
#include "include/onlinesearch.h"
#include "include/options.h"
#include "include/releaselists.h"
#include "include/sfo.h"
#include "include/strings.h"
#include "include/terminal.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <conio.h> // For getch()
#include "include/dirent.h"
#define DIR_SEPARATOR '\\'
#else
#include <dirent.h>
#include <signal.h>
#include <stdio_ext.h> // For __fpurge(); requires the GNU C Library
#define DIR_SEPARATOR '/'
#endif

char format_string[MAX_FORMAT_STRING_LEN] =
  "%title% [%dlc%] [{v%app_ver%}] [%title_id%] [%release_group%] [%release%] [%backport%]";
struct custom_category custom_category =
  {"Game", "Update", "DLC", "App", "Other"};
char *tags[100];
int tagc;
int first_run = 1;

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
  char new_basename[MAX_FILENAME_LEN]; // Used to build the new PKG file's name
  char *basename; // "filename" without path
  char lowercase_basename[MAX_FILENAME_LEN];
  char *path; // "filename" without file
  char tag_release_group[MAX_TAG_LEN + 1] = "";
  char tag_release[MAX_TAG_LEN + 1] = "";
  int spec_chars_current, spec_chars_total;
  int paramc;
  struct sfo_parameter *params;

  // Internal pattern variables
  char *app = NULL;
  char *app_ver = NULL;
  char *backport = NULL;
  char *category = NULL;
  char *content_id = NULL;
  char *dlc = NULL;
  char firmware[9] = "";
  char *game = NULL;
  char *other = NULL;
  char *patch = NULL;
  char region[3] = "";
  char *release_group = NULL;
  char *release = NULL;
  char sdk[6] = "";
  char size[10];
  char title[MAX_TITLE_LEN] = "";
  char *title_backup = NULL;
  char *title_id = NULL;
  char *type = NULL;
  char *version = NULL;

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

  // Print current basename (early)
  if (!option_compact) {
    if (first_run) {
      printf("   \"%s\"\n", basename);
      first_run = 0;
    } else {
      printf("\n   \"%s\"\n", basename);
    }
  }

  // Load SFO parameters into variables
  params = sfo_read(&paramc, filename);
  if (params == NULL) {
    switch (paramc) {
      case 0:
        fprintf(stderr, "Error while opening file \"%s\".\n", filename);
        exit(1);
      case 1:
        fprintf(stderr, "File is not a PKG: \"%s\".\n", filename);
        exit(1);
      case 2:
        fprintf(stderr, "Param.sfo not found in file \"%s\".\n",
          filename);
        exit(1);
    }
    return;
  }
  for (int i = 0; i < paramc; i++) {
    if (strcmp(params[i].name, "APP_VER") == 0) {
      if (option_leading_zeros == 1) {
        app_ver = params[i].string;
      } else if (params[i].string[0] == '0') {
        app_ver = params[i].string + 1;
      } else {
        app_ver = params[i].string;
      }
    } else if (strcmp(params[i].name, "CATEGORY") == 0) {
      category = params[i].string;
      if (strcmp(params[i].string, "gd") == 0) {
        type = custom_category.game;
        game = type;
      } else if (strstr(params[i].string, "gp") != NULL) {
        type = custom_category.patch;
        patch = type;
      } else if (strcmp(params[i].string, "ac") == 0) {
        type = custom_category.dlc;
        dlc = type;
      } else if (params[i].string[0] == 'g' && params[i].string[1] == 'd') {
        type = custom_category.app;
        app = type;
      } else {
        type = custom_category.other;
        other = type;
      }
    } else if (strcmp(params[i].name, "CONTENT_ID") == 0) {
      content_id = params[i].string;
      region[0] = content_id[0];
      region[1] = content_id[1];
      region[2] = '\0';
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
      sprintf(firmware, "%08x", *params[i].integer);
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
  if ((type == custom_category.patch && strcmp(sdk, "5.05") == 0)
      || strwrd(lowercase_basename, "backport")
      || strwrd(lowercase_basename, "bp")) {
    backport = "Backport";
  }

  // Detect releases
  release_group = get_release_group(lowercase_basename);
  release = get_uploader(lowercase_basename);

  // Get file size in GiB
  if (strstr(format_string, "%size%")) {
    FILE *file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    snprintf(size, sizeof(size), "%.2f GiB", ftello(file) / 1073741824.0);
    fclose(file);
  }

  // Option "online"
  if (option_online == 1) {
    if (option_compact) {
      search_online(content_id, title, 1); // Silent search
    } else {
      search_online(content_id, title, 0);
    }
  }

  // Option "mixed-case"
  if (option_mixed_case == 1) {
    mixed_case(title);
  }

  // User input loop
  int first_loop = 1;
  char c;
  while(1) {
    /***************************************************************************
     * Build new file name
     **************************************************************************/

    // Replace pattern variables
    strcpy(new_basename, format_string);
    // Types first, as they may contain other pattern variables
    strreplace(new_basename, "%type%", type);
    strreplace(new_basename, "%app%", app);
    strreplace(new_basename, "%dlc%", dlc);
    strreplace(new_basename, "%game%", game);
    strreplace(new_basename, "%other%", other);
    strreplace(new_basename, "%patch%", patch);

    strreplace(new_basename, "%app_ver%", app_ver);
    strreplace(new_basename, "%backport%", backport);
    strreplace(new_basename, "%category%", category);
    strreplace(new_basename, "%content_id%", content_id);
    strreplace(new_basename, "%region%", region);
    strreplace(new_basename, "%firmware%", firmware);
    if (tag_release_group[0] != '\0') {
      strreplace(new_basename, "%release_group%", tag_release_group);
    } else {
      strreplace(new_basename, "%release_group%", release_group);
    }
    if (tag_release[0] != '\0') {
      strreplace(new_basename, "%release%", tag_release);
    } else {
      strreplace(new_basename, "%release%", release);
    }
    strreplace(new_basename, "%sdk%", sdk);
    if (strstr(format_string, "%size%"))
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
    replace_illegal_characters(new_basename);

    spec_chars_total = count_spec_chars(new_basename);

    // Replace misused special characters
    if (strreplace(new_basename, "＆", "&")) spec_chars_current--;
    if (strreplace(new_basename, "’", "'")) spec_chars_current--;
    if (strreplace(new_basename, " ", " ")) spec_chars_current--;
    if (strreplace(new_basename, "Ⅲ", "III")) spec_chars_current--;

    // Replace potentially annoying special characters
    if (strreplace(new_basename, "™_", "_")) spec_chars_current--;
    if (strreplace(new_basename, "™", " ")) spec_chars_current--;
    if (strreplace(new_basename, "®_", "_")) spec_chars_current--;
    if (strreplace(new_basename, "®", " ")) spec_chars_current--;
    if (strreplace(new_basename, "–", "-")) spec_chars_current--;

    spec_chars_current = count_spec_chars(new_basename);

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

    /**************************************************************************/

    // Print current basename (late)
    if (option_compact) {
      if (strcmp(basename, new_basename) == 0) {
        goto exit;
      } else {
        if (first_run) {
          printf("   \"%s\"\n", basename);
          first_run = 0;
        } else if (first_loop) {
          printf("\n   \"%s\"\n", basename);
        }
      }
      first_loop = 0;
    }

    // Print new basename
    printf("=> \"%s\"", new_basename);

    // Print number of special characters
    if (option_verbose) {
      printf(BRIGHT_YELLOW " (%d/%d)" RESET, spec_chars_current,
        spec_chars_total);
    } else if (spec_chars_current) {
      printf(BRIGHT_YELLOW " (%d)" RESET, spec_chars_current);
    }
    printf("\n");

    // Exit if new basename too long
    if (strlen(new_basename) > MAX_FILENAME_LEN - 1) {
      fprintf(stderr, "New filename too long (%lu/%d) characters).\n",
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

    /***************************************************************************
     * Interactive prompt
     **************************************************************************/

    // Clear stdin immediately
    #ifndef _WIN32
    __fpurge(stdin);
    #endif

    // Read user input
    printf("OK? [Y]es [N]o [A]ll [E]dit [T]ag [M]ix [O]nline [R]eset [C]hars [S]FO [Q]uit: ");
    do {
      #ifdef _WIN32
      c = getch();
      #else
      c = getchar();
      #endif
    } while (strchr("ynaetmorcsqb", c) == NULL);
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
        ;
        char backup[MAX_TITLE_LEN];
        strcpy(backup, title);
        reset_terminal();
        printf("\nEnter new title: ");
        fgets(title, MAX_TITLE_LEN, stdin);
        title[strlen(title) - 1] = '\0'; // Remove Enter character
        // Remove entered control characters
        for (int i = 0; i < strlen(title); i++) {
          if (iscntrl(title[i])) {
            memmove(&title[i], &title[i + 1], strlen(title) - i);
            i--;
          }
        }
        // Restore title if nothing has been entered
        if (title[0] == '\0') {
          strcpy(title, backup);
          printf("Using title \"%s\".\n", title);
        }
        printf("\n");
        raw_terminal();
        break;
      case 't': // [T]ag: let user enter release groups or releases
        ;
        char tag[MAX_TAG_LEN] = "";
        printf("\nEnter new tag: ");
        scan_string(tag, MAX_TAG_LEN, "", get_tag);
        char *result;
        if (tag[0] != '\0') {
          if ((result = get_release_group(tag)) != NULL) {
            printf("Using \"%s\" as release group.\n", result);
            strncpy(tag_release_group, result, MAX_TAG_LEN);
            tag_release_group[MAX_TAG_LEN] = '\0';
          } else if ((result = get_uploader(tag)) != NULL) {
            printf("Using \"%s\" as release.\n", result);
            strncpy(tag_release, result, MAX_TAG_LEN);
            tag_release[MAX_TAG_LEN] = '\0';
          } else if (strcmp(tag, "Backport") == 0) {
            if (backport) {
              printf("Backport tag disabled.\n");
              backport = NULL;
            } else {
              printf("Backport tag enabled.\n");
              backport = "Backport";
            }
          } else {
            printf("Using \"%s\" as release.\n", tag);
            strncpy(tag_release, tag, MAX_TAG_LEN);
            tag_release[MAX_TAG_LEN] = '\0';
          }
        }
        printf("\n");
        break;
      case 'm': // [M]ix: convert title to mixed-case letter format
        mixed_case(title);
        printf("\nConverted letter case to mixed-case style.\n\n");
        break;
      case 'o': // [O]nline: search the PlayStation store online for metadata
        printf("\n");
        search_online(content_id, title, 0);
        printf("\n");
        break;
      case 'r': // [R]eset: undo all changes
        strcpy(title, title_backup);
        printf("\nTitle has been reset to \"%s\".\n", title_backup);
        if (tag_release_group[0] != '\0') {
          printf("Tagged release group \"%s\" has been reset.\n",
            tag_release_group);
          tag_release_group[0] = '\0';
        }
        if (tag_release[0] != '\0') {
          printf("Tagged release \"%s\" has been reset.\n", tag_release);
          tag_release[0] = '\0';
        }
        printf("\n");
        break;
      case 'c': // [C]hars: reveal all non-printable characters
        printf("\nOriginal: \"%s\"\nRevealed: \"", title);
        int count = 0;
        for (int i = 0; i < strlen(title); i++) {
          if (isprint(title[i])) {
            printf("%c", title[i]);
          } else {
            count++;
            printf(BRIGHT_YELLOW "#%d" RESET,
              title[i]);
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
      case 'b': // [B]ackport: toggle backport tag
        if (backport) {
          backport = NULL;
          printf("\nBackport tag disabled.\n\n");
        } else {
          backport = "Backport";
          printf("\nBackport tag enabled.\n\n");
        }
        break;
    }
  }

  exit:
  free(params);
  free(path);
}

int main(int argc, char *argv[]) {
  initialize_terminal();
  raw_terminal();

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

  #ifdef _WIN32
  reset_terminal();
  #endif

  exit(0);
}
