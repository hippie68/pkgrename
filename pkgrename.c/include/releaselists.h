#ifndef RELEASELISTS_H
#define RELEASELISTS_H

// Searches the argument for known release groups and returns the first match
char *get_release_group(char *string);

// Searches the argument for known uploaders and returns the first match
char *get_uploader(char *string);

char *get_tag(char *string);

#endif
