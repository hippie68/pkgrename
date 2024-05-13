#ifndef STRINGS_H
#define STRINGS_H

void trim_string(char *string, char *ltrim, char *rtrim);
char *strwrd(const char *string, char *word);
char *strreplace(char *string, char *search, char *replace);
void mixed_case(char *string);
int lower_strcmp(char *string1, char *string2);

#endif
