#ifndef COMMON
#define COMMON

#include <stddef.h>

#ifdef _WIN32
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

#define MAX_FILENAME_LEN 256 // exFAT file name limit (+1)
#define MAX_FORMAT_STRING_LEN 512
#define MAX_TITLE_LEN 128 // https://www.psdevwiki.com/ps4/Param.sfo#TITLE
#define HOMEPAGE_LINK "https://github.com/hippie68/pkgrename"
#define SUPPORT_LINK "https://github.com/hippie68/pkgrename/issues"

extern char format_string[MAX_FORMAT_STRING_LEN];
extern char placeholder_char;
struct custom_category {
  char *game;
  char *patch;
  char *dlc;
  char *app;
  char *other;
};

extern struct custom_category custom_category;

#endif
