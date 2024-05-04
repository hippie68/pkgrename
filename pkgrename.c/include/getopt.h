// A thread-safe, more intuitive alternative to getopt_long() that features
// word-wrapping help output and nested subcommands.
// https://github.com/hippie68/getopt

#ifndef GETOPT_H
#define GETOPT_H

/// Interface ------------------------------------------------------------------

#include <stdio.h>

struct option {
    int index;         // The option's unique identifier.
                       // A single alphanumeric ASCII character literal means
                       // this is the option's short name. Any other positive
                       // value means the option does not have a short name.
                       // A negative value means the option is a subcommand.
    char *name;        // The option's unique long name (without leading "--")
                       // or the subcommand's name.
    char *arg;         // A single option-argument or the subcommand's operands,
                       // as they are supposed to appear in the help screen
                       // (e.g. "ARG", "[ARG]", "<arg>", ...).
                       // Square brackets mean the argument is optional.
    char *description; // The option's or the subcommand's description.
};

#define HIDEOPT &var_HIDEOPT // Hides options if used as value for .description.

// Return values: 0 when done, '?' on error, otherwise an option's .index value.
int getopt(int *argc, char **argv[], char **optarg, const struct option opts[]);

// Prints the specified option array's options.
// Returns 1 if the array has subcommands, otherwise 0.
int print_options(FILE *stream, const struct option opts[]);

// Prints the specified option array's subcommands.
void print_subcommands(FILE *stream, const struct option opts[]);

/* Quick example:
int main(int argc, char *argv[])
{
    static struct option opts[] = {
        { 'h', "help", NULL, "Print help information and quit." },
        { 0 }
    };

    int opt;
    char *optarg;
    while ((opt = getopt(&argc, &argv, &optarg, opts)) != 0) {
        switch (opt) {
            case 'h':
                print_options(stdout, opts);
                return 0;
            case '?': // Error
                return 1;
        }
    }
}
*/

/// Implementation -------------------------------------------------------------

#include <string.h>

// Customizable values used by print_options() and print_subcommands().
#ifndef GETOPT_LINE_MAXLEN
#define GETOPT_LINE_MAXLEN 80 // Max. length of lines of text output.
#endif
#ifndef GETOPT_BLCK_MINLEN
#define GETOPT_BLCK_MINLEN 30 // Min. length of an option's description block.
#endif

// Error messages printed by getopt().
#define ERR_SHRTOPT_NEEDARG "Option -%c requires an argument.\n"
#define ERR_LONGOPT_NEEDARG "Option --%s requires an argument.\n"
#define ERR_SHRTOPT_UNKNOWN "Unknown option: -%c\n"
#define ERR_LONGOPT_UNKNOWN "Unknown option: --%s\n"
#define ERR_COMMAND_UNKNOWN "Unknown subcommand: %s\n"
#define ERR_SHRTOPT_HATEARG "Option -%c doesn't allow an argument.\n"
#define ERR_LONGOPT_HATEARG "Option --%s doesn't allow an argument.\n"

static char var_HIDEOPT; // Dummy variable to make the HIDEOPT pointer unique.

// Left-shifts array elements by 1, moving the first element to the end.
static void shift(char *argv[])
{
    char *first_element = argv[0];
    while (argv[1] != NULL) {
        argv[0] = argv[1];
        argv++;
    }
    argv[0] = first_element;
}

// Return values: 0 when done, '?' on error, otherwise an option's .index value.
int getopt(int *argc, char **argv[], char **optarg, const struct option opts[])
{
    if (*argc <= 0 || argv == NULL || *argv == NULL || **argv == NULL
        || optarg == NULL || opts == NULL)
        goto parsing_finished;

    if (*optarg)
        *optarg = NULL;

    // Advance parsing.
    if (*argc > 0) {
        (*argv)++;
        (*argc)--;
    }

    if (*argc == 0 || **argv == NULL)
        goto parsing_finished;

    char *argp; // Pointer used to probe the command line arguments.

start:
    argp = **argv;
    if (*argp == '-') { // Option
        argp++;
        if (*argp == '-') { // Long option
            argp++;

            // Handle "end of options" (--).
            if (*argp == '\0') {
                (*argv)++;
                (*argc)--;

                // Shift-hide all remaining operands.
                if ((*argv)[*argc] != NULL)
                    while (*argc) {
                        shift(*argv);
                        (*argc)--;
                    }

                goto parsing_finished;
            }

            // Save attached option-argument, if it exists.
            *optarg = strchr(argp, '=');
            if (*optarg) {
                **optarg = '\0';
                (*optarg)++;
            }

            const struct option *opt = opts;
            while (opt->index) {
                if (opt->name != NULL && strcmp(opt->name, argp) == 0) {
                    if (*optarg) {
                        if (opt->arg == NULL) {
                            fprintf(stderr, ERR_LONGOPT_HATEARG, opt->name);
                            return '?';
                        }
                    } else if (opt->arg && opt->arg[0] != '[') {
                        (*argv)++;
                        (*argc)--;
                        if (*argc == 0 || (*optarg = **argv) == NULL) {
                            fprintf(stderr, ERR_LONGOPT_NEEDARG, opt->name);
                            return '?';
                        }
                    }
                    return opt->index;
                }
                opt++;
            }
            fprintf(stderr, ERR_LONGOPT_UNKNOWN, argp);
            return '?';
        } else if (*argp == '\0') { // A single "-"
            goto not_an_option; // Treat it as an operand.
        } else { // Short option group
            const struct option *opt = opts;
            while (opt->index) {
                if (opt->index == *argp) { // Short option is known.
                    if (*(argp + 1) == '\0') { // No characters are attached.
                        if (opt->arg && opt->arg[0] != '[') {
                            (*argv)++;
                            (*argc)--;
                            if (*argc == 0 || (*optarg = **argv) == NULL) {
                                fprintf(stderr, ERR_SHRTOPT_NEEDARG, *argp);
                                return '?';
                            }
                        }
                    } else { // Option has attached characters.
                        if (opt->arg) {
                            *optarg = argp + 1;
                        } else {
                            if (*(argp + 1) == '-') { // Unwanted argument.
                                fprintf(stderr, ERR_SHRTOPT_HATEARG, *argp);
                                return '?';
                            }
                            *argp = '-';
                            **argv = argp; // Scan here again next round.
                            (*argv)--;
                            (*argc)++;
                        }
                    }
                    return opt->index;
                }
                opt++;
            }
            fprintf(stderr, ERR_SHRTOPT_UNKNOWN, *argp);
            return '?';
        }
    } else { // Either operand or subcommand
not_an_option:
        ;

        // Return if operand is a subcommand.
        const struct option *opt = opts;
        int subcommands_exist = 0;
        while (opt->index) {
            if (opt->index < 0) {
                if (opt->name && strcmp(opt->name, argp) == 0)
                    return opt->index;
                subcommands_exist = 1;
            }
            opt++;
        }

        // Don't move operand if subcommands exist.
        if (subcommands_exist) {
            fprintf(stderr, ERR_COMMAND_UNKNOWN, argp);
            return '?';
        }

        // Move operand to the end of argv[] and hide it for now.
        shift(*argv);
        (*argc)--; // Hide it.

        if (*argc == 0)
            goto parsing_finished;

        goto start;
    }

parsing_finished:
    // Unhide any previously hidden operands.
    while ((*argv)[*argc] != NULL)
        (*argc)++;

    return 0;
}

// Helper function for print_block().
static int next_word_size(char *p)
{
    char *word_end = p;
    do {
        word_end++;
    } while (*word_end != ' ' && *word_end != '\0');
    return word_end - p;
}

// Uses word-wrapping to print an indented string over multiple lines.
static void print_block(FILE *stream, char *str, int indent, int start)
{
    if (start >= GETOPT_LINE_MAXLEN) {
        putc('\n', stderr);
        start = 0;
    }

    while (*str) {
        // Print line indentation.
        for (int i = 0, n = indent - start; i < n; i++)
            putc(' ', stream);

        int chars_left = GETOPT_LINE_MAXLEN - (start > indent ? start : indent);

        // Skip non-intended first characters.
        if (str[0] == ' ' || str[0] == '\n')
            str++;

        // Print all words that fit in the current line.
        int next_len;
        while (*str && (next_len = next_word_size(str)) <= chars_left) {
            for (int i = 0; i < next_len; i++) {
                if (*str == '\n')
                    goto next_line;

                putc(*str, stream);
                str++;
            }
            chars_left -= next_len;
        }

next_line:
        putc('\n', stream);
        start = 0;
    }
}

// Returns 1 if an option index is a valid short option character, otherwise 0.
static inline int is_short_name(int index)
{
    char *range = "abcdefghijklmnopqrstuvwxyz"
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "0123456789";
    for (int i = 0; i < 62; i++) {
        if (index == range[i])
            return 1;
    }

    return 0;
}

// Prints a single option.
// Adjustments here must be done in print_opts(), too.
static void print_opt(FILE *stream, const struct option *opt, int indent)
{
    if (opt->description == HIDEOPT)
        return;

    if (opt->index > 0) {
        int short_name = is_short_name(opt->index);

                                   // 1 2 3 4 5 6 7 8 9 0 1 2 3
        int len = fprintf(stream, "  %c%c%s%s%s%s%s%s%s%s%s%s%s",
    /* 1 */ short_name ? '-' : ' ',
    /* 2 */ short_name ? opt->index : ' ',
    /* 3 */ short_name && !opt->name && opt->arg ? opt->arg[0] == '[' ? opt->arg : " " : "",
    /* 4 */ short_name && !opt->name && opt->arg && opt->arg[0] != '[' ? opt->arg : "",
    /* 5 */ short_name && opt->name ? "," : short_name ? "" : " ",
    /* 6 */ opt->name ? " --" : "",
    /* 7 */ opt->name ? opt->name : "",
    /* 8 */ opt->name && opt->arg && opt->arg[0] != '[' ? " " : "",
    /* 9 */ opt->name && opt->arg && opt->arg[0] != '[' ? opt->arg : "",
    /* 0 */ opt->name && opt->arg && opt->arg[0] == '[' ? "[" : "",
    /* 1 */ opt->name && opt->arg && opt->arg[0] == '[' ? "=" : "",
    /* 2 */ opt->name && opt->arg && opt->arg[0] == '[' ? opt->arg + 1 : "",
    /* 3 */ opt->description ? "  " : "");

        if (opt->description)
            print_block(stream, opt->description, indent, len);
        else
            putc('\n', stream);
    }
}

// Prints the specified option array's options.
// Returns 1 if the array has subcommands, otherwise 0.
int print_options(FILE *stream, const struct option opts[])
{
    const struct option *opt = opts;
    int has_subcommands = 0;

    // Calculate option description indentation.
    // Adjustments here must be done in print_opt(), too.
    int indent = 0;
    while (opt->index) {
        if (opt->index > 0) {
            int len = 6; // Minimum indentation, "  -x" and "  .description".
            if (opt->name)
                len += 4 + strlen(opt->name); // 4: ", --"
            if (opt->arg) {
                len += strlen(opt->arg);
                if (opt->name || opt->arg[0] == '[')
                    len++;
            }
            if (len > indent && len <= GETOPT_LINE_MAXLEN - GETOPT_BLCK_MINLEN)
                indent = len;
        }
        opt++;
    }

    // Print options.
    opt = opts;
    while (opt->index) {
        if (opt->index < 0)
            has_subcommands = 1;
        else
            print_opt(stream, opt, indent);
        opt++;
    }

    return has_subcommands;
}

// Prints a single subcommand.
// Adjustments here must be done in print_subcommands(), too.
static void print_subcmd(FILE *stream, const struct option *opt, int indent)
{
    if (opt->index == 0 || opt->description == HIDEOPT)
        return;

    if (opt->index < 0) {
        int len = fprintf(stream, "  %s%s%s%s",
            opt->name ? opt->name : "",
            opt->arg ? " " : "",
            opt->arg ? opt->arg : "",
            opt->description ? "  ": "");
        if (opt->description)
            print_block(stream, opt->description, indent, len);
        else
            putc('\n', stream);
    }
}

// Prints the specified option array's subcommands.
void print_subcommands(FILE *stream, const struct option opts[])
{
    const struct option *opt = opts;

    // Calculate subcommand description indentation.
    // Adjustments here must be done in print_subcmd(), too.
    int indent = 0;
    while (opt->index) {
        if (opt->index < 0) {
            int len = 4; // Minimum indentation, "  .name" and "  .description".
            if (opt->name)
                len += strlen(opt->name);
            if (opt->arg)
                len += 1 + strlen(opt->arg);
            if (len > indent && len <= GETOPT_LINE_MAXLEN - GETOPT_BLCK_MINLEN)
                indent = len;
        }
        opt++;
    }

    // Print subcommands.
    opt = opts;
    while (opt->index) {
        if (opt->index < 0)
            print_subcmd(stream, opt, indent);
        opt++;
    }
}

#endif
