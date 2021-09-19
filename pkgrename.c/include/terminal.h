#ifndef TERMINAL_H
#define TERMINAL_H

// Needs to be called before other functions
// Parameter is the function to be called on SIGINT
void initialize_terminal();

// Sets terminal to noncanonical input mode (= no need to press enter)
void raw_terminal();

// Resets terminal
void reset_terminal();

// Reads user input and stores it in buffer "string"
void scan_string(char *string, int max_size, char *default_string);

#endif
