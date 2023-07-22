#include "../include/common.h"
#include "../include/releaselists.h"
#include "../include/strings.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

struct rls_list {
  char *name;
  char *alt_name;
};

// List of release groups; [0]: Name, [1]: Additional search string
// (Additional search strings commonly appear in original file names.)
static struct rls_list release_groups[] = {
  {"AUGETY"},
  {"BigBlueBox"},
  {"BlaZe", "blz"},
  {"CAF"},
  {"DarKmooN"},
  {"DUPLEX"},
  {"GCMR"},
  {"HOODLUM"},
  {"HR"},
  {"iNTERNAL"},
  {"JRP"},
  {"KOTF"},
  {"LeveLUp"},
  {"LiGHTFORCE", "lfc"},
  {"MarvTM"},
  {"MOEMOE", "moe-"},
  {"PiKMiN"},
  {"Playable"},
  {"PRELUDE"},
  {"PROTOCOL"},
  {"RESPAWN"},
  {"SharpHD"},
  {"TCD"},
  {"UNLiMiTED"},
  {"WaLMaRT"},
  {"WaYsTeD"},
  {NULL}
};

// List of uploaders
static struct rls_list uploaders[] = {
  {"Arczi"},
  {"CyB1K", "rayku22"},
  {"High Speed", "highspeed33"},
  {"OPOISSO893", "opoisso"},
  {"SeanP2500", "seanp"},
  {"TKJ13"},
  {"VikaCaptive"},
  {"Whitehawkx"},
  {NULL}
};

// Detects release group in a string and returns a pointer containing the group.
char *get_release_group(char *string) {
  struct rls_list *p = release_groups;

  while (p->name != NULL) {
    if (strwrd(string, p->name)) return p->name;
    if (p->alt_name != NULL && strwrd(string, p->alt_name)) return p->name;
    p++;
  }

  return NULL;
}

// Detects uploader in a string and returns a pointer containing the uploader.
char *get_uploader(char *string) {
  struct rls_list *p = uploaders;

  // Check user-specified tags first, so they can override default tags
  for (int i = 0; i < tagc; i++) {
    if (strwrd(string, tags[i])) return tags[i];
  }

  while (p->name != NULL) {
    if (strwrd(string, p->name)) return p->name;
    if (p->alt_name != NULL && strwrd(string, p->alt_name)) return p->name;
    p++;
  }

  return NULL;
}

// Returns 1 if str2 fully matches the beginning of str1 (case-insensitive).
static int strings_match(char *str1, char *str2) {
  if (strlen(str2) == 0) return 0;
  if (strlen(str2) > strlen(str1)) {
    return 0;
  }

  for (size_t i = 0; i < strlen(str2); i++) {
    if (tolower(str2[i]) != tolower(str1[i])) return 0;
  }

  return 1;
}

// Used as autocomplete function for scan_string() (in terminal.c).
// Returns the name of a tag if it is found in "string".
char *get_tag(char *string) {
  struct rls_list *p;

  p = release_groups;
  while (p->name != NULL) {
    if (strings_match(p->name, string))
      return p->name;
    p++;
  }

  // Check user-specified tags first, so they can override default uploaders
  for (int i = 0; i < tagc; i++) {
    if (strings_match(tags[i], string)) return tags[i];
  }

  p = uploaders;
  while (p->name != NULL) {
    if (strings_match(p->name, string))
      return p->name;
    p++;
  }

  if (strings_match("Backport", string)) return "Backport";

  return NULL;
}

// Prints a list of all known release groups and releases.
void print_database() {
  struct rls_list *p;

  p = release_groups;
  printf("Known release groups:\n");
  while (p->name != NULL) {
    printf("  - %s\n", p->name);
    p++;
  }

  p = uploaders;
  printf("\nKnown releases:\n");
  while (p->name != NULL) {
    printf("  - %s\n", p->name);
    p++;
  }
}
