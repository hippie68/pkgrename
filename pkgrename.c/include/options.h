#ifndef OPTIONS_H
#define OPTIONS_H

// Default options
extern int option_force;
extern int option_fw;
extern int option_mixed_case;
extern int option_no_placeholder;
extern int option_no_to_all;
extern int option_leading_zeros;
extern int option_online;
extern int option_recursive;
extern int option_underscores;
extern int option_verbose;
extern int option_yes_to_all;

void print_usage(void);
void parse_options(int argc, char *argv[]);

#endif
