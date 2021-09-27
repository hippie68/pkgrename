#include "../include/common.h"
#include "../include/getopts.h"
#include "../include/options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Default options
int option_force = 0;
int option_mixed_case = 0;
int option_no_placeholder = 0;
int option_no_to_all = 0;
int option_leading_zeros = 0;
int option_online = 0;
int option_recursive = 0;
int option_underscores = 0;
int option_verbose = 0;
int option_yes_to_all = 0;

void print_version(void) {
  printf("Release date: %s %s\n", __DATE__, __TIME__);
  printf("Get the latest version at "
    "\"%s\".\n", HOMEPAGE_LINK);
  printf("Report bugs, request features, or add missing data at "
    "\"%s\".\n", SUPPORT_LINK);
}

void print_usage(void) {
  printf(
  "Usage: pkgrename [options] [file|directory ...]\n"
  "\n"
  "Renames PS4 PKGs to match a file name pattern. The default pattern is:\n"
  "\"%%title%% [%%type%%] [%%title_id%%] [%%release_group%%] [%%release%%] [%%backport%%]\"\n"
  "\n"
  "Pattern variables:\n"
  "------------------\n"
  "  Name             Example\n"
  "  ----------------------------------------------------------------------\n"
  "  %%app_ver%%        \"1.50\"\n"
  "  %%backport%%       \"Backport\" (*)\n"
  "  %%category%%       \"gp\"\n"
  "  %%content_id%%     \"EP4497-CUSA05571_00-00000000000GOTY1\"\n"
  "  %%firmware%%       \"4.70\"\n"
  "  %%release_group%%  \"PRELUDE\" (*)\n"
  "  %%release%%        \"John Doe\" (*)\n"
  "  %%sdk%%            \"4.50\"\n"
  "  %%size%%           \"0.11 GiB\"\n"
  "  %%title%%          \"The Witcher 3: Wild Hunt – Game of the Year Edition\"\n"
  "  %%title_id%%       \"CUSA05571\"\n"
  "  %%type%%           \"Update 1.50\" (**)\n"
  "  %%version%%        \"1.00\"\n"
  "\n"
  "  (*) Backports not targeting 5.05 are detected by searching file names for\n"
  "  strings \"BP\", \"Backport\", and \"BACKPORT\". The same principle applies\n"
  "  to release groups and releases.\n"
  "\n"
  "  (**) %%type%% is %%category%% mapped to \"Game,Update %%app_ver%%,DLC,App\".\n"
  "  These default strings, up to 5, can be changed via option \"--set-type\":\n"
  "    --set-type \"Game,Patch,DLC,App,Other\" (no spaces before or after commas)\n"
  "\n"
  "  Only the first occurence of each pattern variable will be parsed.\n"
  "  After parsing, empty pairs of brackets, empty pairs of parentheses, and any\n"
  "  remaining curly braces (\"[]\", \"()\", \"{\", \"}\") will be removed.\n"
  "\n"
  "Curly braces expressions:\n"
  "-------------------------\n"
  "  Pattern variables and other strings can be grouped together by surrounding\n"
  "  them with curly braces. If an inner pattern variable turns out to be empty,\n"
  "  the whole curly braces expression will be removed.\n"
  "\n"
  "  Example 1 - %%firmware%% is empty:\n"
  "    \"%%title%% [FW %%firmware%%]\"   => \"Example DLC [FW ].pkg\"  WRONG\n"
  "    \"%%title%% [{FW %%firmware%%}]\" => \"Example DLC.pkg\"        CORRECT\n"
  "\n"
  "  Example 2 - %%firmware%% has a value:\n"
  "    \"%%title%% [{FW %%firmware%%}]\" => \"Example Game [FW 7.55].pkg\"\n"
  "\n"
  "Handling of special characters:\n"
  "-------------------------------\n"
  "  - For exFAT compatibility, some characters are replaced by a placeholder\n"
  "    character (default: underscore).\n"
  "  - Some special characters like copyright symbols are automatically removed\n"
  "    or replaced by more common alternatives.\n"
  "  - Numbers appearing in parentheses behind a file name indicate the presence\n"
  "    of non-ASCII characters.\n"
  "\n"
  "Interactive prompt:\n"
  "-------------------\n"
  "  - [Y]es     Rename the file as seen.\n"
  "  - [N]o      Skip the file and drops all changes.\n"
  "  - [A]ll     Same as yes, but also for all future files.\n"
  "  - [E]dit    Prompt to manually edit the title.\n"
  "  - [T]ag     Prompt to enter a release group or a release.\n"
  "  - [M]ix     Convert the letter case to mixed-case style.\n"
  "  - [O]nline  Search the PS Store online for title information.\n"
  "  - [R]eset   Undo all changes.\n"
  "  - [C]hars   Reveal special characters in the title.\n"
  "  - [S]FO     Show file's param.sfo information.\n"
  "  - [Q]uit    Exit the program.\n"
  "  - [B]       (Hidden) Toggle the \"Backport\" tag.\n"
  "\n");
  printf(
  "Options:\n"
  "--------\n"
  "  -f, --force           Force-prompt even when file names match.\n"
  "  -h, --help            Print this help screen.\n"
  "  -0, --leading-zeros   Show leading zeros in pattern variables %%app_ver%%,\n"
  "                        %%firmware%%, %%sdk%%, and %%version%%.\n"
  "  -m, --mixed-case      Automatically apply mixed-case letter style.\n"
  "      --no-placeholder  Hide characters instead of using placeholders.\n"
  "  -n, --no-to-all       Do not prompt; do not actually rename any files.\n"
  "  -o, --online          Automatically search online for %%title%%.\n"
  "  -p, --pattern x       Set the file name pattern to string x.\n"
  "      --placeholder x   Set the placeholder character to x.\n"
  "      --print-database  Print all current database entries.\n"
  "  -r, --recursive       Traverse subdirectories recursively.\n"
  "      --set-type x      Set %%type%% mapping to 5 comma-separated strings x.\n"
  "  -u, --underscores     Use underscores instead of spaces in file names.\n"
  "  -v, --verbose         Display additional infos.\n"
  "      --version         Print release date.\n"
  "  -y, --yes-to-all      Do not prompt; rename all files automatically.\n"
  );
}

// Option functions
static inline void optf_force() { option_force = 1; }
static inline void optf_help() {
  print_usage();
  exit(0);
}
static inline void optf_leading_zeros() { option_leading_zeros = 1; }
static inline void optf_mixed_case() { option_mixed_case = 1; }
static inline void optf_no_placeholder() { option_no_placeholder = 1; }
static inline void optf_no_to_all() { option_no_to_all = 1; }
static inline void optf_online() { option_online = 1; }
static inline void optf_pattern(char *pattern) {
  size_t len = strlen(pattern);
  if (len >= MAX_FORMAT_STRING_LEN) {
    fprintf(stderr, "Option %s: Pattern too long (%ld/%d characters).\n",
      pattern, len, MAX_FORMAT_STRING_LEN - 1);
    exit(1);
  }
  strcpy(format_string, pattern);
}
static inline void optf_placeholder(char *placeholder) { placeholder_char = placeholder[0]; }
static inline void optf_print_database() {
  extern void print_database();
  print_database();
  exit(0);
}
static inline void optf_recursive() { option_recursive = 1; }
static inline void optf_set_type(char *argument) {
  char backup[strlen(argument) + 1];
  strcpy(backup, argument);
  custom_category.game = strtok(argument, ",");
  custom_category.patch = strtok(NULL, ",");
  custom_category.dlc = strtok(NULL, ",");
  custom_category.app = strtok(NULL, ",");
  custom_category.other = strtok(NULL, ",");
}
static inline void optf_underscores() { option_underscores = 1; }
static inline void optf_verbose() { option_verbose = 1; }
static inline void optf_version() {
  print_version();
  exit(0);
}
static inline void optf_yes_to_all() { option_yes_to_all = 1; }

void parse_options(int argc, char *argv[]) {
  struct OPTION options[] = {
    {'f', "force",          0, optf_force},
    {'h', "help",           0, optf_help},
    {'0', "leading-zeros",  0, optf_leading_zeros},
    {'m', "mixed-case",     0, optf_mixed_case},
    {'n', "no-to-all",      0, optf_no_to_all},
    {0,   "no-placeholder", 0, optf_no_placeholder},
    {'o', "online",         0, optf_online},
    {'p', "pattern",        1, optf_pattern},
    {0,   "placeholder",    1, optf_placeholder},
    {0,   "print-database", 0, optf_print_database},
    {'r', "recursive",      0, optf_recursive},
    {0,   "set-type",       1, optf_set_type},
    //{'t', "tag",            1, optf_set_tag}, // TODO: Comma-separated list as argument; contains release groups and/or releases to rename the file with;
                                              // If a tag is unknown, let the user know to report his favorite tags at GitHub
                                              // Also put this into the interactive loop as [T]ag
    /*
    {0,   "set-keeper-1",   1, optf_set_keeper_1}, // TODO: Custom string to keep; perhaps with set-keeperx and __func; e.g. useful for keeping "Backport" in the file name
    {0,   "set-keeper-2",   1, optf_set_keeper_2},
    {0,   "set-keeper-3",   1, optf_set_keeper_3},
    */
    {'u', "underscores",    0, optf_underscores},
    {'v', "verbose",        0, optf_verbose},
    {0,   "version",        0, optf_version},
    {0,   "yes-to-all",     0, optf_yes_to_all},
    {EOF}
  };
  if (get_opts(argc, argv, options) != 0) exit(1);
}
