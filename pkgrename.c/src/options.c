#include "../include/common.h"
#include "../include/colors.h"
#include "../include/getopt.h"
#include "../include/options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int option_compact;
int option_disable_colors;
int option_force;
int option_force_backup;
int option_mixed_case;
int option_no_placeholder;
int option_no_to_all;
char option_language_number[3];
int option_leading_zeros;
int option_online;
int option_override_tags;
int option_query;
int option_recursive;
char *option_tag_separator;
int option_underscores;
int option_verbose;
int option_yes_to_all;

enum long_only_options {
    OPT_DISABLE_COLORS = 256,
    OPT_NO_PLACEHOLDER,
    OPT_OVERRIDE_TAGS,
    OPT_PLACEHOLDER,
    OPT_PRINT_LANGS,
    OPT_PRINT_TAGS,
    OPT_SET_TYPE,
    OPT_TAGFILE,
    OPT_TAGS,
    OPT_TAG_SEPARATOR,
    OPT_VERSION,
};

static struct option opts[] = {
    { 'c',                "compact",        NULL,      "Hide files that are already renamed." },
#ifndef _WIN32
    { OPT_DISABLE_COLORS, "disable-colors", NULL,      "Disable colored text output." },
#endif
    { 'f',                "force",          NULL,      "Force-prompt even when file names match." },
    { 'h',                "help",           NULL,      "Print this help screen." },
    { 'l',                "language",       "LANG",    "If the PKG supports it, use the language specified by language code LANG (see --print-languages) to retrieve the PKG's title." },
    { '0',                "leading-zeros",  NULL,      "Show leading zeros in pattern variables %app_ver%, %firmware%, %merged_ver%, %sdk%, %true_ver%, %version%." },
    { 'm',                "mixed-case",     NULL,      "Automatically apply mixed-case letter style." },
    { OPT_NO_PLACEHOLDER, "no-placeholder", NULL,      "Hide characters instead of using placeholders." },
    { 'n',                "no-to-all",      NULL,      "Do not prompt; do not actually rename any files. This can be used to do a test run." },
    { 'o',                "online",         NULL,      "Automatically search online for %title%." },
    { OPT_OVERRIDE_TAGS,  "override-tags",  NULL,      "Make changelog release tags take precedence over existing file name tags." },
    { 'p',                "pattern",        "PATTERN", "Set the file name pattern to string PATTERN." },
    { OPT_PLACEHOLDER,    "placeholder",    "X",       "Set the placeholder character to X." },
    { OPT_PRINT_LANGS,    "print-languages", NULL,     "Print available language codes." },
    { OPT_PRINT_TAGS,     "print-tags",     NULL,      "Print all built-in release tags." },
    { 'q',                "query",          NULL,      "For scripts/tools: print file name suggestions, one per line, without renaming the files. A successful query returns exit code 0." },
    { 'r',                "recursive",      NULL,      "Traverse subdirectories recursively." },
    { OPT_SET_TYPE,       "set-type",       "CATEGORIES", "Set %type% mapping to comma-separated string CATEGORIES (see section \"Pattern variables\")." },
    { OPT_TAGFILE,        "tagfile",        "FILE",    "Load additional %release% tags from text file FILE, one tag per line." },
    { OPT_TAGS,           "tags",           "TAGS",    "Load additional %release% tags from comma-separated string TAGS (no spaces before or after commas)." },
    { OPT_TAG_SEPARATOR,  "tag-separator",  "SEP",     "Use the string SEP instead of commas to separate multiple release tags." },
    { 'u',                "underscores",    NULL,      "Use underscores instead of spaces in file names." },
    { 'v',                "verbose",        NULL,      "Display additional infos." },
    { OPT_VERSION,        "version",        NULL,      "Print the current pkgrename version." },
    { 'y',                "yes-to-all",     NULL,      "Do not prompt; rename all files automatically." },
    { 0 }
};

void print_version(void)
{
    printf("Version 1.09 BETA, build date: %s\n", __DATE__);
    printf("Get the latest version at "
        "\"%s\".\n", HOMEPAGE_LINK);
    printf("Report bugs, request features, or add missing tags at "
        "\"%s\".\n", SUPPORT_LINK);
}

void print_prompt_help(void)
{
    fputs(
        "  - [Y]es      Rename the file as seen.\n"
        "  - [N]o       Skip the file and drop all changes.\n"
        "  - [A]ll      Same as yes, but also for all future files.\n"
        "  - [E]dit     Prompt to manually edit the title.\n"
        "  - [T]ag      Prompt to enter a release group or a release.\n"
        "  - [M]ix      Convert the letter case to mixed-case style.\n"
        "  - [O]nline   Search the PS Store online for title information.\n"
        "  - [R]eset    Undo all changes.\n"
        "  - [C]hars    Reveal special characters in the title.\n"
        "  - [S]FO      Show file's param.sfo information.\n"
        "  - [L]og      Print existing changelog data.\n"
        "  - [H]elp     Print help.\n"
        "  - [Q]uit     Exit the program.\n"
        "  - [B]        Toggle the \"Backport\" tag.\n"
        "  - [P]        Toggle changelog patch detection for the current PKG.\n"
        "  - Shift-[T]  Remove all release tags.\n"
        "  - Backspace  Go back to the previous PKG.\n"
        "  - Space      Return to the current PKG.\n"
        , stdout);
}

void print_usage(void)
{
    fputs(
        "Usage: pkgrename [OPTIONS] [FILE|DIRECTORY ...]\n"
        "\n"
        "Renames PS4 PKGs to match a file name pattern. The default pattern is:\n"
        "\"%title% [%dlc%] [{v%app_ver%}{ + v%merged_ver%}] [%title_id%] [%release_group%] [%release%] [%backport%]\"\n"
        "\n"
        "Pattern variables:\n"
        "------------------\n"
        "  Name             Example\n"
        "  ----------------------------------------------------------------------\n"
        "  %app%            \"App\"\n"
        "  %app_ver%        \"4.03\"\n"
        "  %backport%       \"Backport\" (1)\n"
        "  %category%       \"gp\"\n"
        "  %content_id%     \"EP4497-CUSA05571_00-00000000000GOTY1\"\n"
        "  %dlc%            \"DLC\"\n"
        "  %fake%           \"Fake\" (5)\n"
        "  %fake_status%    \"Fake\" (5)\n"
        "  %file_id%        \"EP4497-CUSA05571_00-00000000000GOTY1-A0403-V0100\"\n"
        "  %firmware%       \"10.01\"\n"
        "  %game%           \"Game\"\n"
        "  %merged_ver%     \"\" (2)\n"
        "  %msum%           \"3E57B0\" (3)\n"
        "  %other%          \"Other\"\n"
        "  %patch%          \"Update\"\n"
        "  %region%         \"EU\"\n"
        "  %release_group%  \"PRELUDE\" (1)\n"
        "  %release%        \"John Doe\" (1)\n"
        "  %retail%         \"\" (5)\n"
        "  %sdk%            \"4.50\"\n"
        "  %size%           \"19.34 GiB\"\n"
        "  %title%          \"The Witcher 3: Wild Hunt – Game of the Year Edition\"\n"
        "  %title_id%       \"CUSA05571\"\n"
        "  %true_ver%       \"4.03\" (2)\n"
        "  %type%           \"Update\" (4)\n"
        "  %version%        \"1.00\"\n"
        "\n"
        "  (1) Backports not targeting 5.05 are detected by searching file names for the\n"
        "  words \"BP\" and \"Backport\" (case-insensitive). The same principle applies to\n"
        "  release groups and releases.\n"
        "\n"
        "  (2) Patches and apps merged with patches are detected by searching PKG files\n"
        "  for changelog information. If a patch is found, both %merged_ver% and\n"
        "  %true_ver% are the patch version. If no patch is found or if patch detection\n"
        "  is disabled (command [P]), %merged_ver% is empty and %true_ver% is %app_ver%.\n"
        "  %merged_ver% is always empty for non-app PKGs.\n"
        "\n"
        "  (3) A checksum that indicates whether game and update PKGs that have the\n"
        "  same Title ID are compatible with each other (\"married\"). This pattern\n"
        "  variable will be empty for PKGs of other types.\n"
        "\n"
        "  (4) %type% is %category% mapped to \"Game,Update,DLC,App,Other\".\n"
        "  These five default strings can be changed via option \"--set-type\", e.g.:\n"
        "    --set-type \"Game,Patch %app_ver%,DLC,-,-\" (no spaces before or after commas)\n"
        "  Each string must have a value. To hide a category, use the value \"-\".\n"
        "  %app%, %dlc%, %game%, %other%, and %patch% are mapped to their corresponding\n"
        "  %type% values. They will be displayed if the PKG is of that specific category.\n"
        "\n"
        "  (5) These pattern variables depend on the type of the PKG:\n"
        "  PKG type           %fake%   %retail%  %fake_status%\n"
        "  \"Fake\" PKG (FPKG)  Fake     <empty>   Fake\n"
        "  Retail PKG         <empty>  Retail    Retail\n"
        "\n"
        "  After parsing, empty pairs of brackets, empty pairs of parentheses, and any\n"
        "  remaining curly braces (\"[]\", \"()\", \"{\", \"}\") will be removed.\n"
        "\n"
        "Curly braces expressions:\n"
        "-------------------------\n"
        "  Pattern variables and other strings can be grouped together by surrounding\n"
        "  them with curly braces. If an inner pattern variable turns out to be empty,\n"
        "  the whole curly braces expression will be removed.\n"
        "\n"
        "  Example 1 - %firmware% is empty:\n"
        "    \"%title% [FW %firmware%]\"   => \"Example DLC [FW ].pkg\"  WRONG\n"
        "    \"%title% [{FW %firmware%}]\" => \"Example DLC.pkg\"        CORRECT\n"
        "\n"
        "  Example 2 - %firmware% has a value:\n"
        "    \"%title% [{FW %firmware%}]\" => \"Example Game [FW 7.55].pkg\"\n"
        "\n"
        "Handling of special characters:\n"
        "-------------------------------\n"
        "  - For exFAT compatibility, some characters are replaced by a placeholder\n"
        "    character (default: underscore).\n"
        "  - Some special characters like copyright symbols are automatically removed\n"
        "    or replaced by more common alternatives.\n"
        "  - Numbers appearing in parentheses behind a file name indicate the presence\n"
        "    of non-ASCII characters.\n"
        "\n"
        "Interactive prompt:\n"
        "-------------------\n"
        , stdout);
    print_prompt_help();
    fputs("\nOptions:\n"
            "--------\n", stdout);
    print_options(stdout, opts);
}

// Option functions ------------------------------------------------------------

static inline void optf_pattern(char *pattern)
{
    size_t len = strlen(pattern);
    if (len >= MAX_FORMAT_STRING_LEN) {
#ifdef _WIN64
        fprintf(stderr, "Pattern too long (%llu/%d characters).\n", len,
#elif defined(_WIN32)
        fprintf(stderr, "Pattern too long (%u/%d characters).\n", len,
#else
        fprintf(stderr, "Pattern too long (%lu/%d characters).\n", len,
#endif
        MAX_FORMAT_STRING_LEN - 1);
        exit(EXIT_FAILURE);
    }
    strcpy(format_string, pattern);
}

static struct lang {
    unsigned char number;
    char *name;
    char *identifier;
} langs[] = {
    { 0, "Japanese", "jp" },
    { 1, "English (United States)", "en" },
    { 2, "French (France)", "fr" },
    { 3, "Spanish (Spain)", "es" },
    { 4, "German", "de" },
    { 5, "Italian", "it" },
    { 6, "Dutch", "nl" },
    { 7, "Portuguese (Portugal)", "pt" },
    { 8, "Russian", "ru" },
    { 9, "Korean", "ko" },
    { 10, "Chinese (traditional)", "zh_t" },
    { 11, "Chinese (simplified)", "zh_s" },
    { 12, "Finnish", "fi" },
    { 13, "Swedish", "sv" },
    { 14, "Danish", "da" },
    { 15, "Norwegian", "no" },
    { 16, "Polish", "pl" },
    { 17, "Portuguese (Brazil)", "pt-br" },
    { 18, "English (United Kingdom)", "en-gb" },
    { 19, "Turkish", "tr" },
    { 20, "Spanish (Latin America)", "es-la" },
    { 21, "Arabic", "ar" },
    { 22, "French (Canada)", "fr-ca" },
    { 23, "Czech", "cs" },
    { 24, "Hungarian", "hu" },
    { 25, "Greek", "el" },
    { 26, "Romanian", "ro" },
    { 27, "Thai", "th" },
    { 28, "Vietnamese", "vi" },
    { 29, "Indonesian", "in" },
};

static int compar_lang(const void *l1, const void *l2)
{
    return strcmp(((struct lang *) l1)->name, ((struct lang *) l2)->name);
}

static inline void optf_print_languages(void)
{
    qsort(langs, sizeof(langs) / sizeof(langs[0]), sizeof(langs[0]),
            compar_lang);
    printf("Code\tLanguage\n\n");
    for (size_t i = 0; i < sizeof(langs) / sizeof(struct lang); i++) {
        printf("%s\t%s\n", langs[i].identifier, langs[i].name);
    }
}

static inline void optf_set_type(char *argument)
{
    // Exit if wrong number of arguments
    int comma_count = 0;
    for (size_t i = 0; i < strlen(argument); i++) {
        if (argument[i] == ',')
            comma_count++;
    }
    if (comma_count != 4) {
        fprintf(stderr,
            "Option --set-type needs exactly 5 comma-separated arguments.\n");
        exit(EXIT_FAILURE);
    }

    // Parse arguments
    custom_category.game = strtok(argument, ",");
    if (strcmp(custom_category.game, "-") == 0)
        custom_category.game = "";
    custom_category.patch = strtok(NULL, ",");
    if (strcmp(custom_category.patch, "-") == 0)
        custom_category.patch = "";
    custom_category.dlc = strtok(NULL, ",");
    if (strcmp(custom_category.dlc, "-") == 0)
        custom_category.dlc = "";
    custom_category.app = strtok(NULL, ",");
    if (strcmp(custom_category.app, "-") == 0)
        custom_category.app = "";
    custom_category.other = strtok(NULL, ",");
    if (strcmp(custom_category.other, "-") == 0)
        custom_category.other = "";
}

static inline void optf_tagfile(char *file_name)
{
    FILE *tagfile = fopen(file_name, "r");
    if (!tagfile) {
        fprintf(stderr, "Option --tagfile: File not found: \"%s\".\n",
            file_name);
        exit(EXIT_FAILURE);
    }

    char buf[MAX_TAG_LEN + 1];
    while (fgets(buf, sizeof buf, tagfile) && tagc < MAX_TAGS) {
        buf[strcspn(buf, "\r\n")] = '\0';
        if (buf[0] == '\0')
            continue;
        tags[tagc] = realloc(tags[tagc], strlen(buf) + 1);
        memcpy(tags[tagc], buf, strlen(buf) + 1);
        tagc++;
    }

    fclose(tagfile);
}

static inline void optf_tags(char *taglist)
{
    char *p = strtok(taglist, ",");
    while (p && tagc < MAX_TAGS) {
        tags[tagc] = realloc(tags[tagc], strlen(p) + 1);
        memcpy(tags[tagc], p, strlen(p) + 1);
        tagc++;
        p = strtok(NULL, ",");
    }
}

// -----------------------------------------------------------------------------

void parse_options(int *argc, char **argv[])
{
    int opt;
    char *optarg;
    while ((opt = getopt(argc, argv, &optarg, opts)) != 0) {
        switch (opt) {
            case 'c':
                option_compact = 1;
                break;
#ifndef _WIN32
            case OPT_DISABLE_COLORS:
                option_disable_colors = 1;
                break;
#endif
            case 'f':
                option_force = 1;
                option_force_backup = 1;
                break;
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
            case 'l':
                for (size_t i = 0; i < sizeof(langs) / sizeof(langs[0]); i++) {
                    if (strcmp(optarg, langs[i].identifier) == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
                        snprintf(option_language_number, 3, "%02d", langs[i].number);
#pragma GCC diagnostic pop
                        goto language_found;
                    }
                }

                fprintf(stderr, "Unknown language code: %s\n", optarg);
                exit(EXIT_FAILURE);
language_found:
                break;
            case '0':
                option_leading_zeros = 1;
                break;
            case 'm':
                option_mixed_case = 1;
                break;
            case OPT_NO_PLACEHOLDER:
                option_no_placeholder = 1;
                break;
            case 'n':
                option_no_to_all = 1;
                break;
            case 'o':
                option_online = 1;
                break;
            case OPT_OVERRIDE_TAGS:
                option_override_tags = 1;
                break;
            case 'p':
                optf_pattern(optarg);
                break;
            case OPT_PLACEHOLDER:
                placeholder_char = optarg[0];
                break;
            case OPT_PRINT_LANGS:
                optf_print_languages();
                exit(EXIT_SUCCESS);
            case OPT_PRINT_TAGS:
                ;
                extern void print_database();
                print_database();
                exit(EXIT_SUCCESS);
            case 'q':
                option_query = 1;
                break;
            case 'r':
                option_recursive = 1;
                break;
            case OPT_SET_TYPE:
                optf_set_type(optarg);
                break;
            case OPT_TAGFILE:
                optf_tagfile(optarg);
                break;
            case OPT_TAGS:
                optf_tags(optarg);
                break;
            case OPT_TAG_SEPARATOR:
                option_tag_separator = optarg;
                break;
            case 'u':
                option_underscores = 1;
                break;
            case 'v':
                option_verbose = 1;
                break;
            case OPT_VERSION:
                print_version();
                exit(EXIT_SUCCESS);
            case 'y':
                option_yes_to_all = 1;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
}
