#include "../include/releaselists.h"

#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <shlwapi.h>
#define strcasestr StrStrIA
#endif

struct rls_list {
  char *name;
  char *search;
  char *alt_search;
};

// List of release groups ([0]: Name, [1-2]: Search strings):
static struct rls_list release_groups[] = {
  {"BigBlueBox", "bigbluebox"},
  {"BlaZe", "blaze", "blz"},
  {"CAF", "caf"},
  {"DarKmooN", "darkmoon"},
  {"DUPLEX", "duplex"},
  {"GCMR", "gcmr"},
  {"HOODLUM", "hoodlum"},
  {"HR", "hr"},
  {"iNTERNAL", "internal"},
  {"JRP", "jrp"},
  {"KOTF", "kotf"},
  {"LeveLUp", "levelup"},
  {"LiGHTFORCE", "lightforce", "lfc"},
  {"MarvTM", "marvtm"},
  {"MOEMOE", "moemoe", "moe-"},
  {"Playable", "playable"},
  {"PRELUDE", "prelude"},
  {"PROTOCOL", "protocol"},
  {"RESPAWN", "respawn"},
  {"SharpHD", "sharphd"},
  {"TCD", "tcd"},
  {"UNLiMiTED", "unlimited"},
  {"WaLMaRT", "walmart"},
  {"WaYsTeD", "waysted"},
  {NULL}
};

// List of uploaders
static struct rls_list uploaders[] = {
  {"Arczi", "arczi"},
  {"CyB1K", "cyb1k"},
  {"OPOISSO893", "opoisso893"},
  {"SeanP2500", "seanp2500"},
  {NULL}
};

// Detects release group in a string and returns a pointer containing the group
char *get_release_group(char *string) {
  struct rls_list *p = release_groups;
  while (p->name != NULL) {
    if (p->search != NULL && strstr(string, p->search) != NULL)
      return p->name;
    if (p->alt_search != NULL && strstr(string, p->alt_search) != NULL)
      return p->name;
    p++;
  }

  return NULL;
}

// Detects uploader in a string and returns a pointer containing the uploader
char *get_uploader(char *string) {
  struct rls_list *p = uploaders;
  while (p->name != NULL) {
    if (p->search != NULL && strstr(string, p->search) != NULL)
      return p->name;
    if (p->alt_search != NULL && strstr(string, p->alt_search) != NULL)
      return p->name;
    p++;
  }

  return NULL;
}
