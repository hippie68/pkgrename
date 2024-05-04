#ifndef RELEASELISTS_H
#define RELEASELISTS_H

// Searches the argument for known release groups and returns the first match.
char *get_release_group(char *string);

// Searches the argument for known releases and returns the first match.
int get_release(char **release, const char *string);

// Used as autocomplete function for scan_string() (in terminal.c).
// Returns the name of a tag if it is found in "string".
char *get_tag(char *string);

// Searches a changelog buffer for all known release tags and prints them.
void print_changelog_tags(const char *changelog_buf);

#endif
