#ifndef CHARACTERS_H
#define CHARACTERS_H

extern char *illegal_characters;
extern char placeholder_char;

int is_in_set(char c, char *set);
int count_spec_chars(char *string);
void replace_illegal_characters(char *string);

#endif
