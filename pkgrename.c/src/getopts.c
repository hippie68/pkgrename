#include "../include/getopts.h"

int noptc;
char **nopts;

// Parses an array of options and runs functions associated with options
// Returns 0 on success, 1 on error
int get_opts(int argc, char *argv[], struct OPTION options[]) {
  nopts = malloc(sizeof (char *) * argc);
  noptc = 0;
  int look_for_options = 1;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    // Look for options
    if (look_for_options == 1 && strlen(argv[i]) > 1) {
      if (strcmp(argv[i], "--") == 0) {
        look_for_options = 0;
        continue;
      }
      // Argument is an option
      if (argv[i][0] == '-' && strlen(argv[i]) > 1) {

        // Argument is a long option ("--abc")
        if (argv[i][1] == '-') {
          for (int i_opt = 0; ; i_opt++) {

            // Error if option is unknown
            if (options[i_opt].short_name == EOF) {
              fprintf(stderr, "Unknown option: %s\n", argv[i]);
              return 1;
            }

            if (options[i_opt].long_name == NULL) continue;

            if (strcmp(options[i_opt].long_name, argv[i] + 2) == 0) {
              //printf("Found known long option: %s\n", argv[i]); // DEBUG

              // Run option's function with argument
              if (options[i_opt].needs_argument == 1) {
                i++;
                if (i < argc) {
                  options[i_opt].f(argv[i]);
                  break;
                } else {
                  fprintf(stderr, "Option --%s needs an argument.\n",
                    options[i_opt].long_name);
                  return 1;
                }
              }

              // Run option's function without argument
              if (options[i_opt].f != NULL) options[i_opt].f(NULL);

              break;
            }
          }

        // Argument is a short option ("-a") or block of short options ("-abc")
        } else {
          size_t block_len = strlen(argv[i]);

          // Compare all of the block's characters with known short options
          for (int pos = 1; pos < block_len; pos++) {
            for (int i_opt = 0; ; i_opt++) {
              // Character is a known short option
              if (options[i_opt].short_name == argv[i][pos]) {
                //printf("Found known short option: -%c\n", argv[i][pos]); //DEBUG

                // Run option's function with argument
                if (options[i_opt].needs_argument == 1) {
                  if (pos == block_len - 1) { // Character must be the last one
                    i++;
                    if (i < argc) {
                      options[i_opt].f(argv[i]);
                      break; // Compare block's next character
                    }
                  }

                  // Still here? Argument not existing or specified incorrectly
                  fprintf(stderr, "Option -%c needs an argument.\n",
                    options[i_opt].short_name);
                  return 1;
                }

                // Run option's function without argument
                else if (options[i_opt].f != NULL) {
                  options[i_opt].f(NULL);
                }

                break; // Compare block's next character

              // Error if no options left to compare with
              } else if (options[i_opt].short_name == EOF) {
                fprintf(stderr, "Unknown option: -%c\n", argv[i][pos]);
                return 1;
              }
            }
          }
        }
        continue; // Check next command line argument
      }
    }

    // Argument is not an option
    nopts[noptc] = argv[i];
    noptc++;
  }

  nopts = realloc(nopts, sizeof (char *) * noptc);

  return 0;
}

// Debug function that prints the results of a split_options() call
void print_nopts() {
  printf("noptc: %d\n", noptc);
  for (int i = 0; i < noptc; i++) {
    printf("nopts[%d]: %s\n", i, nopts[i]);
  }
}
