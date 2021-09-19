#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Used to store all options
// Use it like this:
// struct OPTION options[] = {
//   { "-h", "--help", false, optf_help, "Display this help information" },
//   { "-o", NULL, true, optf_output, "Set the output file" },
//   { NULL, "--version", false, optf_help, "Display version information" },
//   { EOF }
// };
struct OPTION {
  int short_name; // Short option, e.g. 'a' to represent "-a"
  char *long_name; // Long option, e.g. "option" to represent "--option"
  int needs_argument; // 1 if option needs an argument, 0 if not
  void (*f)(char *argument); // Calling get_opts() will call function f and
                             // provide the option's argument automatically
  char *description; // Optional
  int done; // Used internally to run each option only once
};

// Splits arguments into opts ("options") and nopts ("non-options").
// Returns 0 on success, 1 on error.
int get_opts(int argc, char *argv[], struct OPTION options[]);

// After calling get_opts(), non-options are stored in
// these variables:
int noptc;
char **nopts;  // nopts[0] will be argv[0]

// Prints non-options
void print_nopts();
