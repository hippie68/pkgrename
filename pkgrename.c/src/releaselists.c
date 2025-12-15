#ifndef _WIN32
#define _GNU_SOURCE // For strcasecmp().
#endif

#include "../include/colors.h"
#include "../include/common.h"
#include "../include/options.h"
#include "../include/releaselists.h"
#include "../include/strings.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct rls_list {
    char *name;
    char *alt_name;
};

// List of release groups; [0]: Name, [1]: Additional search string
// (Additional search strings commonly appear in original file names.)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static struct rls_list release_groups[] = {
    {"AUGETY"},
    {"BigBlueBox"},
    {"BlaZe", "blz"},
    {"CAF"},
    {"DarKmooN"},
    {"DUPLEX"},
    {"GCMR"},
    {"HOODLUM"},
    {"HR"},
    {"iNTERNAL"},
    {"JRP"},
    {"KOTF"},
    {"LeveLUp"},
    {"LiGHTFORCE", "lfc"},
    {"MarvTM"},
    {"MOEMOE", "moe-"},
    {"PiKMiN"},
    {"Playable"},
    {"PRELUDE"},
    {"PROTOCOL"},
    {"RESPAWN"},
    {"SharpHD"},
    {"TCD"},
    {"UNLiMiTED"},
    {"WaLMaRT"},
    {"WaYsTeD"},
    {NULL}
};

// List of releases
static struct rls_list releases[] = {
    {"Arczi"},
    {"CyB1K", "rayku22"},
    {"Fugazi", "mrboot"},
    {"High Speed", "highspeed33"},
    {"OPOISSO893", "opoisso"},
    {"SeanP2500", "seanp"},
    {"TKJ13"},
    {"TRIFECTA"},
    {"VikaCaptive"},
    {"Whitehawkx"},
    {"xmrallx"},
    {NULL}
};
#pragma GCC diagnostic pop

#ifdef _WIN32
// MinGW is missing strcasestr().
static char *strcasestr(const char *haystack, const char *needle)
{
    if (*haystack == '\0' || *needle == '\0')
        return NULL;

    size_t haystack_len = strlen(haystack);
    size_t needle_len = strlen(needle);
    if (haystack_len < needle_len)
        return NULL;
    size_t diff_len = haystack_len - needle_len;

    do {
        if (tolower(*haystack) == tolower(*needle)) {
            const char *a = haystack;
            const char *b = needle;
            do {
                ++a;
                ++b;
                if (*b == '\0')
                    return (char *) haystack;
            } while (tolower(*a) == tolower(*b));
        }
        ++haystack;
    } while (diff_len-- > 0);

    return NULL;
}
#endif

// Detects release group in a string and returns a pointer containing the group.
char *get_release_group(char *string)
{
    struct rls_list *p = release_groups;

    while (p->name != NULL) {
        if (strwrd(string, p->name))
            return p->name;
        if (p->alt_name != NULL && strstr(string, p->alt_name))
            return p->name;
        p++;
    }

    return NULL;
}

static int compar_func(const void *s1, const void *s2)
{
    return strcasecmp(*(char **) s1, *(char **) s2);
}

// Companion function for get_release().
// Returns 1 if a tag matches one of the user-provided tags.
static int matches_user_tag(const char *tag)
{
    for (int i = 0; i < tagc; i++) {
        if (strcasecmp(tags[i], tag) == 0)
            return 1;
    }

    return 0;
}

// Detects one or multiple releases in a string and stores a pointer to the
// resulting comma-separated string in <release>. Returns the number of found
// unique matches.
int get_release(char **release, const char *string)
{
    static char *found[MAX_TAGS + 1];
    int n_found = 0;
    static char *retval;

    // Check user-specified tags first, so they can override built-in tags.
    for (int i = 0; i < tagc; i++)
        if (strwrd(string, tags[i]))
            found[n_found++] = tags[i];

    // Check built-in tags.
    struct rls_list *p = releases;
    while (p->name != NULL) {
        if (!matches_user_tag(p->name) && (strwrd(string, p->name)
            || (p->alt_name != NULL && strwrd(string, p->alt_name))
            || (strcmp(p->name, "Fugazi") == 0 && strcasestr(string, "fxd"))))
            found[n_found++] = p->name;
        p++;
    }

    if (n_found == 1) {
        *release = found[0];
    } else if (n_found > 1) {
        // Return multiple releases as a comma-separated string.
        qsort(found, n_found, sizeof(char *), compar_func);
        if (retval == NULL)
            retval = calloc(MAX_TAG_LEN, 1);
        strncpy(retval, found[0], MAX_TAG_LEN);
        for (int i = 1; i < n_found; i++) {
            strncat(retval, tag_separator, MAX_TAG_LEN - strlen(retval));
            strncat(retval, found[i], MAX_TAG_LEN - strlen(retval));
        }
        *release = retval;
    }

    return n_found;
}

// Returns 1 if str2 fully matches the beginning of str1 (case-insensitive).
static int strings_match(char *str1, char *str2)
{
    if (strlen(str2) == 0)
        return 0;
    if (strlen(str2) > strlen(str1))
        return 0;

    for (size_t i = 0; i < strlen(str2); i++) {
        if (tolower(str2[i]) != tolower(str1[i]))
            return 0;
    }

    return 1;
}

// Used as autocomplete function for scan_string() (in terminal.c).
// Returns the name of a tag if it is found in "string".
char *get_tag(char *string)
{
    struct rls_list *p;

    // Ignore already entered tags that are separated by commas.
    char *last_comma = strrchr(string, ',');
    if (last_comma != NULL) {
        string = last_comma + 1;
        while (*string == ' ')
            string++;
    }

    p = release_groups;
    while (p->name != NULL) {
        if (strings_match(p->name, string))
            return p->name;
        p++;
    }

    // Check user-specified tags first, so they can override built-in tags.
    for (int i = 0; i < tagc; i++) {
        if (strings_match(tags[i], string))
            return tags[i];
    }

    // Check built-in tags.
    p = releases;
    while (p->name != NULL) {
        if (strings_match(p->name, string))
            return p->name;
        p++;
    }

    if (strings_match("Backport", string))
        return "Backport";

    return NULL;
}

// Replaces all commas in a release tag with a custom string.
// The buffer <tag> is stored in must be of length MAX_TAG_LEN.
void replace_commas_in_tag(char *tag, const char *string)
{
//    size_t taglen = strlen(tag); // Initial length
    size_t stringlen = strlen(string);

    for (size_t i = 0; i < strlen(tag); i++) {
        if (tag[i] == ',') {
            // Only do it if there's enough space left in the tag buffer.
            if (strlen(tag) + stringlen - 1 >= MAX_TAG_LEN)
                return;

            memmove(tag + i + stringlen, tag + i + 1, strlen(tag + i));
            memcpy(tag + i, string, stringlen);
            i += stringlen;
        }
    }
}

// Searches a changelog buffer for all known release tags and prints them.
void print_changelog_tags(const char *changelog_buf)
{
    int first_match = 0;

    // User-specified tags.
    for (int i = 0; i < tagc; i++) {
        if (strwrd(changelog_buf, tags[i])) {
            if (first_match == 0) {
                fputs("Release tags found:\n", stdout);
                first_match = 1;
                set_color(BRIGHT_YELLOW, stdout);
            }
            printf("%s (user tag)\n", tags[i]);
        }
    }

    // Built-in tags.
    struct rls_list *p = releases;
    while (p->name != NULL) {
        char *name;
        if (strcasestr(changelog_buf, p->name)
            || (p->alt_name && strcasestr(changelog_buf, p->alt_name)))
            name = p->name;
        else
            name = NULL;
        if (name) {
            if (first_match == 0) {
                fputs("Release tags found:\n", stdout);
                first_match = 1;
                set_color(BRIGHT_YELLOW, stdout);
            }
            printf("%s (built-in tag)\n", name);
        }
        p++;
    }

    if (first_match)
        set_color(RESET, stdout);
}

// Prints a list of all built-in release group tags and release tags.
void print_database()
{
    struct rls_list *p;

    p = release_groups;
    printf("Known release groups:\n");
    while (p->name != NULL) {
        printf("  - %s\n", p->name);
        p++;
    }

    p = releases;
    printf("\nKnown releases:\n");
    while (p->name != NULL) {
        printf("  - %s\n", p->name);
        p++;
    }
}
