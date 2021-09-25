#include "../include/colors.h"
#include "../include/common.h"
#include "../include/dirparse.h"
#include "../include/options.h"

#ifdef _WIN32
#include "../include/dirent.h"
#else
#include <dirent.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int first_run;
extern void pkgrename(char *filename);

// Companion function for qsort in parse_directory()
static int qsort_compare_strings(const void *p, const void *q) {
  return strcmp(*(const char **)p, *(const char **)q);
}

// Finds all .pkg files in a directory and runs pkgrename() on them
void parse_directory(char *directory_name) {
  DIR *dir;
  struct dirent *directory_entry;

  int directory_count_max = 100;
  int file_count_max = 100;
  char **directory_names = malloc(sizeof(void *) * directory_count_max);
  char **filenames = malloc(sizeof(void *) * file_count_max);
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
        && strcmp(directory_entry->d_name, "..") != 0 // Exclude system dirs
        && strcmp(directory_entry->d_name, ".") != 0)
      {
        directory_names[directory_count] = malloc(strlen(directory_name) + 1 +
          strlen(directory_entry->d_name) + 1);
        sprintf(directory_names[directory_count], "%s%c%s", directory_name,
          DIR_SEPARATOR, directory_entry->d_name);
        directory_count++;
        if (directory_count == directory_count_max) {
          directory_count_max *= 2;
          directory_names = realloc(directory_names,
            sizeof(void *) * directory_count_max);
        }
      }
    // Entry is .pkg file
    } else {
      char *file_extension = strrchr(directory_entry->d_name, '.');
      if (file_extension != NULL && strcasecmp(file_extension, ".pkg") == 0) {
        // One-time message when entering a new directory that has .pkg files
        if (option_recursive == 1 && file_count == 0) {
          if (first_run) {
            printf("\n");
          } else {
            first_run = 1;
          }
          printf(GRAY "Entering directory \"%s\"\n" RESET, directory_name);
        }
        // Save name in the file list
        filenames[file_count] = malloc(strlen(directory_name) + 1 +
          strlen(directory_entry->d_name) + 1);
        sprintf(filenames[file_count], "%s%c%s", directory_name, DIR_SEPARATOR,
          directory_entry->d_name);
        file_count++;
        if (file_count == file_count_max) {
          file_count_max *= 2;
          filenames = realloc(filenames, sizeof(void *) * file_count_max);
        }
      }
    }
  }

  // Sort the final lists
  qsort(directory_names, directory_count, sizeof(char *),
    qsort_compare_strings);
  qsort(filenames, file_count, sizeof(char *), qsort_compare_strings);

  // First, run pkgrename() on sorted files
  for (int i = 0; i < file_count; i++) {
    pkgrename(filenames[i]);
    free(filenames[i]);
  }

  // Then, parse sorted directories recursively
  if (option_recursive == 1) {
    for (int i = 0; i < directory_count; i++) {
      parse_directory(directory_names[i]);
      free(directory_names[i]);
    }
  }

  free(directory_names);
  free(filenames);

  closedir(dir);
}
