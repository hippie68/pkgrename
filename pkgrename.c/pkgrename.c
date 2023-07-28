#define _FILE_OFFSET_BITS 64

#ifndef _WIN32
#define _GNU_SOURCE // For strcasestr(), which is not standard.
#endif

#include "include/characters.h"
#include "include/colors.h"
#include "include/common.h"
#include "include/getopts.h"
#include "include/onlinesearch.h"
#include "include/options.h"
#include "include/pkg.h"
#include "include/releaselists.h"
#include "include/scan.h"
#include "include/strings.h"
#include "include/terminal.h"

#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <conio.h> // For getch().
#include <shlwapi.h>
#define strcasestr StrStrIA
#define DIR_SEPARATOR '\\'
#else
#include <signal.h>
#include <stdio_ext.h> // For __fpurge(); requires the GNU C Library.
#define DIR_SEPARATOR '/'
#endif

char format_string[MAX_FORMAT_STRING_LEN] =
    "%title% [%dlc%] [{v%app_ver%}{ + v%merged_ver%}] [%title_id%] [%release_group%] [%release%] [%backport%]";
struct custom_category custom_category =
    {"Game", "Update", "DLC", "App", "Other"};
char *tags[MAX_TAGS];
int tagc;
struct scan_result *scan_backup; // Stores a scan result for the space hotkey.

// Debug function that prints the linked list's contents.
void print_linked_list(void)
{
    //pthread_mutex_lock(&mutex);
    fputs("\nLinked list filenames:\n", stderr);
    size_t i = 1;
    struct scan_result *scan = first_scan;
    while (scan && scan != END_OF_SCAN) {
        fprintf(stderr, "%04zu; %p%s:\n", i, (void *) scan, scan == first_scan ? " (first_scan)" : scan == last_scan ? " (last_scan)" : "");
        fprintf(stderr, ".filename: %s\n", scan->filename);
        fprintf(stderr, ".filename_is_allocated: %d\n", scan->filename_is_allocated);
        fprintf(stderr, ".param_sfo: %s\n", scan->param_sfo ? "set" : "NULL");
        fprintf(stderr, ".changelog: %s\n", scan->changelog ? "set" : "NULL");
        fprintf(stderr, ".next: %s\n", scan->next == NULL ? "NULL" : scan->next == last_scan ? "last_scan" : scan->next == END_OF_SCAN ? "END_OF_SCAN" : "set");
        fprintf(stderr, ".next: %p\n", (void *) scan->next);
        fprintf(stderr, ".prev: %s\n", scan->prev ? "set" : "NULL");
        fprintf(stderr, ".prev: %p\n", (void *) scan->prev);
        scan = scan->next;
        ++i;
    }
    //pthread_mutex_unlock(&mutex);
}

static void rename_file(char *filename, char *new_basename, char *path)
{
    FILE *file;
    char new_filename[MAX_FILENAME_LEN];

    strcpy(new_filename, path);
    strcat(new_filename, new_basename);

    file = fopen(new_filename, "rb");

    // File already exists.
    if (file != NULL) {
        fclose(file);
        // Use temporary file to force-rename on (case-insensitive) exFAT.
        if (lower_strcmp(filename, new_filename) == 0) {
            char temp[MAX_FILENAME_LEN];
            strcpy(temp, new_filename);
            strcat(temp, ".pkgrename");
            if (rename(filename, temp)) goto error;
            if (rename(temp, new_filename)) goto error;
        } else {
            fprintf(stderr, "File already exists: \"%s\".\n", new_filename);
            exit(EXIT_FAILURE);
        }
    // File does not exist yet.
    } else {
        if (rename(filename, new_filename)) goto error;
    }

    return;

error:
    fprintf(stderr, "Could not rename file %s.\n", filename);
    exit(EXIT_FAILURE);
}

// Returns the size of a file in bytes or -1 on error.
static ssize_t get_file_size(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        return -1;
    if (fseek(file, 0, SEEK_END)) {
        fclose(file);
        return -1;
    }
    ssize_t ret = ftello(file);
    fclose(file);
    return ret;
}

// Uses information retreived by a previous scan to rename a PS4 PKG file.
// The .next member may not be accessed without a mutex lock.
// Return value:
//   -1 if the scan result must be deleted.
//    1 if the user has requested to go back to the previous scan result.
//    2 if the user has requested to return to the current scan result.
//    0 otherwise.
static int pkgrename(struct scan_result *scan)
{
    static int first_run = 1; // Used to decide when to print newlines.

    // Don't proceed if the scan was aborted due to an error.
    static int error_streak = 0;
    if (scan->error) {
        // Make sure to print consecutive errors as a block.
        if (error_streak == 0) {
            if (first_run == 1)
                first_run = 0;
            else
                putchar('\n');
        }

        print_scan_error(scan);
        //delete_scan_result(scan);
        error_streak = 1;
        return -1;
    } else {
        error_streak = 0;
    }

    if (option_compact == 0) {
        if (first_run == 1)
            first_run = 0;
        else
            putchar('\n');
    }

    char new_basename[MAX_FILENAME_LEN]; // Used to build the new PKG filename.
    char *filename = scan->filename;
    char *basename; // "filename" without path.
    char lowercase_basename[MAX_FILENAME_LEN];
    char *path; // "filename" without file.
    static char *old_path = NULL; // Backup to decide when to print dir changes.
    char tag_release_group[MAX_TAG_LEN + 1] = "";
    char tag_release[MAX_TAG_LEN + 1] = "";
    int spec_chars_current, spec_chars_total;
    int prompted_once = 0;
    int changelog_patch_detection = 1;

    // Internal pattern variables.
    char *app = NULL;
    char *app_ver = NULL;
    char *backport = NULL;
    char *category = NULL;
    char *content_id = NULL;
    char *dlc = NULL;
    char firmware[9] = "";
    char *game = NULL;
    char true_ver_buf[5] = "";
    char *merged_ver = NULL;
    char *other = NULL;
    char *patch = NULL;
    char *region = NULL;
    char *release_group = NULL;
    char *release = NULL;
    char sdk[6] = "";
    char size[10];
    char title[MAX_TITLE_LEN] = "";
    char *title_backup = NULL;
    char *title_id = NULL;
    char *true_ver = NULL;
    char *type = NULL;
    char *version = NULL;

    // Define the file's basename.
    basename = strrchr(filename, DIR_SEPARATOR);
    if (basename == NULL)
        basename = filename;
    else
        basename++;

    // Define the file's path.
    size_t path_len = strlen(filename) - strlen(basename);
    path = malloc(path_len + 1);
    if (path == NULL) {
        fputs("Out of memory.\n", stderr);
        exit(EXIT_FAILURE);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    strncpy(path, filename, path_len);
#pragma GCC diagnostic pop
    path[path_len] = '\0';

    // Print directory if it's different (early).
    if (option_compact == 0) {
        if (old_path && strcmp(path, old_path) != 0) {
            if (option_disable_colors == 0)
                fputs(GRAY, stdout);
            printf("Current directory: %s\n\n", path);
            if (option_disable_colors == 0)
                fputs(RESET, stdout);
        }
        free(old_path);
        old_path = path;
    }

    // Create a lowercase copy of "basename".
    strcpy(lowercase_basename, basename);
    for (size_t i = 0; i < strlen(lowercase_basename); i++)
        lowercase_basename[i] = tolower(lowercase_basename[i]);

    // Print current basename (early).
    if (option_compact == 0)
        printf("   \"%s\"\n", basename);

    // Load PKG data.
    unsigned char *param_sfo = scan->param_sfo;
    char *changelog = scan->changelog;
    // APP_VER
    app_ver = (char *) get_param_sfo_value(param_sfo, "APP_VER");
    if (app_ver && option_leading_zeros == 0 && app_ver[0] == '0')
        app_ver++;
    // CATEGORY
    category = (char *) get_param_sfo_value(param_sfo, "CATEGORY");
    if (category) {
        if (strcmp(category, "gd") == 0) {
            type = custom_category.game;
            game = type;
        } else if (strstr(category, "gp") != NULL) {
            type = custom_category.patch;
            patch = type;
        } else if (strcmp(category, "ac") == 0) {
            type = custom_category.dlc;
            dlc = type;
        } else if (category[0] == 'g' && category[1] == 'd') {
            type = custom_category.app;
            app = type;
        } else {
            type = custom_category.other;
            other = type;
        }
    }
    // CONTENT_ID
    content_id = (char *) get_param_sfo_value(param_sfo, "CONTENT_ID");
    if (content_id) {
        switch (content_id[0]) {
            case 'E': region = "EU"; break;
            case 'H': region = "AS"; break;
            case 'I': region = "IN"; break;
            case 'J': region = "JP"; break;
            case 'U': region = "US"; break;
        }
    }
    // PUBTOOLINFO
    char *pubtoolinfo = (char *) get_param_sfo_value(param_sfo, "PUBTOOLINFO");
    if (pubtoolinfo) {
        char *p = strstr(pubtoolinfo, "sdk_ver=");
        if (p) {
            p += 8;
            memcpy(sdk, p, 4);
            if (sdk[0] == '0' && option_leading_zeros == 0) {
                sdk[0] = sdk[1];
                sdk[1] = '.';
                sdk[4] = '\0';
            } else {
                sdk[4] = sdk[3];
                sdk[3] = sdk[2];
                sdk[2] = '.';
            }
        }
    }
    // SYSTEM_VER
    uint32_t *system_ver = (uint32_t *) get_param_sfo_value(param_sfo,
        "SYSTEM_VER");
    if (system_ver) {
        sprintf(firmware, "%08x", *system_ver);
        if (firmware[0] == '0' && option_leading_zeros == 0) {
            firmware[0] = firmware[1];
            firmware[1] = '.';
            firmware[4] = '\0';
        } else {
            firmware[5] = '\0';
            firmware[4] = firmware[3];
            firmware[3] = firmware[2];
            firmware[2] = '.';
        }
    }
    // TITLE
    title_backup = (char *) get_param_sfo_value(param_sfo, "TITLE");
    if (title_backup) {
        strncpy(title, title_backup, MAX_TITLE_LEN);
        title[MAX_TITLE_LEN - 1] = '\0';
    }
    // TITLE_ID
    title_id = (char *) get_param_sfo_value(param_sfo, "TITLE_ID");
    // VERSION
    version = (char *) get_param_sfo_value(param_sfo, "VERSION");
    if (option_leading_zeros == 0 && version[0] == '0')
        version++;

    // Detect changelog patch level.
    if (changelog && store_patch_version(true_ver_buf, changelog)) {
        if (option_leading_zeros == 0 && true_ver_buf[0] == '0')
            true_ver = true_ver_buf + 1;
        else
            true_ver = true_ver_buf;
        if (category[1] == 'd' && strcmp(true_ver_buf, "01.00") != 0)
            merged_ver = true_ver;
    } else {
        true_ver = app_ver;
    }

    // Detect backport.
    if ((category && category[0] == 'g' && category[1] == 'p'
        && ((option_leading_zeros == 0 && strcmp(sdk, "5.05") == 0 )
        || (option_leading_zeros == 1 && strcmp(sdk, "05.05") == 0)))
        || strstr(lowercase_basename, "backport")
        || strwrd(lowercase_basename, "bp")
        || (changelog && changelog[0] ? strcasestr(changelog, "backport") : 0))
    {
        backport = "Backport";
    }

    // Detect releases.
    release_group = get_release_group(lowercase_basename);
    release = get_uploader(lowercase_basename);
    if (release == NULL && changelog)
        release = get_uploader(changelog);

    // Get file size in GiB.
    if (strstr(format_string, "%size%")) {
        ssize_t file_size = get_file_size(filename);
        if (file_size == -1) {
            fprintf(stderr, "Error while getting the size of file \"%s\".\n",
                filename);
            exit(EXIT_FAILURE);
        }

        snprintf(size, sizeof(size), "%.2f GiB", file_size / 1073741824.0);
    }

    // Option "online".
    if (option_online == 1) {
        if (option_compact)
            search_online(content_id, title, 1); // Silent search.
        else
            search_online(content_id, title, 0);
    }

    // Option "mixed-case".
    if (option_mixed_case == 1)
        mixed_case(title);

    // User input loop.
    int first_loop = 1;
    char c;
    while(1) {
        /***********************************************************************
        * Build new file name
        ***********************************************************************/

        // Replace pattern variables.
        strcpy(new_basename, format_string);
        // Types first, as they may contain other pattern variables.
        strreplace(new_basename, "%type%", type);
        strreplace(new_basename, "%app%", app);
        strreplace(new_basename, "%dlc%", dlc);
        strreplace(new_basename, "%game%", game);
        strreplace(new_basename, "%other%", other);
        strreplace(new_basename, "%patch%", patch);

        strreplace(new_basename, "%app_ver%", app_ver);
        strreplace(new_basename, "%backport%", backport);
        strreplace(new_basename, "%category%", category);
        strreplace(new_basename, "%content_id%", content_id);
        strreplace(new_basename, "%firmware%", firmware);
        strreplace(new_basename, "%merged_ver%", merged_ver);
        strreplace(new_basename, "%region%", region);
        if (tag_release_group[0] != '\0')
            strreplace(new_basename, "%release_group%", tag_release_group);
        else
            strreplace(new_basename, "%release_group%", release_group);
        if (tag_release[0] != '\0')
            strreplace(new_basename, "%release%", tag_release);
        else
            strreplace(new_basename, "%release%", release);
        strreplace(new_basename, "%sdk%", sdk);
        if (strstr(format_string, "%size%"))
            strreplace(new_basename, "%size%", size);
        strreplace(new_basename, "%title%", title);
        strreplace(new_basename, "%title_id%", title_id);
        strreplace(new_basename, "%true_ver%", true_ver);
        strreplace(new_basename, "%version%", version);

        // Remove empty brackets and parentheses, and curly braces.
        while (strreplace(new_basename, "[]", "") != NULL)
            ;
        while (strreplace(new_basename, "()", "") != NULL)
            ;
        while (strreplace(new_basename, "{", "") != NULL)
            ;
        while (strreplace(new_basename, "}", "") != NULL)
            ;

        // Replace illegal characters.
        replace_illegal_characters(new_basename);

        spec_chars_total = count_spec_chars(new_basename);

        // Replace misused special characters.
        if (strreplace(new_basename, "＆", "&")) spec_chars_current--;
        if (strreplace(new_basename, "’", "'")) spec_chars_current--;
        if (strreplace(new_basename, " ", " ")) spec_chars_current--;
        if (strreplace(new_basename, "Ⅲ", "III")) spec_chars_current--;

        // Replace potentially annoying special characters.
        if (strreplace(new_basename, "™_", "_")) spec_chars_current--;
        if (strreplace(new_basename, "™", " ")) spec_chars_current--;
        if (strreplace(new_basename, "®_", "_")) spec_chars_current--;
        if (strreplace(new_basename, "®", " ")) spec_chars_current--;
        if (strreplace(new_basename, "–", "-")) spec_chars_current--;

        spec_chars_current = count_spec_chars(new_basename);

        // Remove any number of repeated spaces.
        while (strreplace(new_basename, "  ", " ") != NULL)
            ;

        // Remove leading whitespace.
        char *p;
        p = new_basename;
        while (isspace(p[0])) p++;
        memmove(new_basename, p, MAX_FILENAME_LEN);

        // Remove trailing whitespace.
        p = new_basename + strlen(new_basename) - 1;
        while (isspace(p[0])) p[0] = '\0';

        // Option --underscores: replace all whitespace with underscores.
        if (option_underscores == 1) {
            p = new_basename;
            while (*p != '\0') {
                if (isspace(*p)) *p = '_';
                p++;
            }
        }

        strcat(new_basename, ".pkg");

        /**********************************************************************/

        // Print current basename (late).
        if (option_compact == 1 && first_loop == 1) {
            if (option_force == 0 && strcmp(basename, new_basename) == 0) {
                goto exit;
            } else {
                if (first_run)
                    first_run = 0;
                else
                    putchar('\n');

                // Print directory if it's different (late).
                if (old_path && strcmp(path, old_path) != 0)
                    printf(GRAY "Current directory: %s\n\n" RESET, path);
                free(old_path);
                old_path = path;

                printf("   \"%s\"\n", basename);
            }
            first_loop = 0;
        }

        // Print new basename.
        printf("=> \"%s\"", new_basename);

        // Print number of special characters.
        if (option_verbose) {
            if (option_disable_colors == 0)
                fputs(BRIGHT_YELLOW, stdout);
            printf(" (%d/%d)", spec_chars_current, spec_chars_total);
            if (option_disable_colors == 0)
                fputs(RESET, stdout);
        } else if (spec_chars_current) {
            if (option_disable_colors == 0)
                fputs(BRIGHT_YELLOW, stdout);
            printf(" (%d)", spec_chars_current);
            if (option_disable_colors == 0)
                fputs(RESET, stdout);
        }
        putchar('\n');

        // Exit if new basename too long.
        if (strlen(new_basename) > MAX_FILENAME_LEN - 1) {
#ifdef _WIN64
            fprintf(stderr, "New filename too long (%llu/%d) characters).\n",
#elif defined(_WIN32)
            fprintf(stderr, "New filename too long (%u/%d) characters).\n",
#else
            fprintf(stderr, "New filename too long (%lu/%d) characters).\n",
#endif
            strlen(new_basename) + 4, MAX_FILENAME_LEN - 1);
            exit(EXIT_FAILURE);
        }

        // Option -n: don't do anything else.
        if (option_no_to_all == 1)
            goto exit;

        // Quit if already renamed.
        if (prompted_once == 0 && option_force == 0
            && strcmp(basename, new_basename) == 0) {
            puts("Nothing to do.");
            goto exit;
        } else {
            prompted_once = 1;
        }

        // Rename now if option_yes_to_all enabled.
        if (option_yes_to_all == 1) {
            rename_file(filename, new_basename, path);
            goto exit;
        }

        /***********************************************************************
         * Interactive prompt
         **********************************************************************/

        // Clear stdin immediately.
#ifndef _WIN32
        __fpurge(stdin);
#endif

        // Read user input.
        fputs("[Y/N/A] [E]dit [T]ag [M]ix [O]nline [R]eset [C]hars [S]FO [L]og [H]elp [Q]uit: ", stdout);
        do {
#ifdef _WIN32
            c = getch();
#else
            c = getchar();
#endif

            // Return to previous file if backspace was pressed.
#ifdef _WIN32
            if (c == 8 && scan->prev != NULL) {
#else
            if (c == 127 && scan->prev != NULL) {
#endif
                putchar('\n');
                return 1;
            }

            // Return to current file if space was pressed.
            if (c == 32 && scan_backup != NULL) {
                putchar('\n');
                return 2;
            }

        } while (strchr("ynaetmorcshqblp", c) == NULL);
        printf("%c\n", c);

        // Evaluate user input,
        switch (c) {
            case 'y': // [Y]es: rename the file.
                rename_file(filename, new_basename, path);
                goto exit;
            case 'n': // [No]: skip file
                goto exit;
            case 'a': // [A]ll: rename files automatically.
                option_yes_to_all = 1;
                rename_file(filename, new_basename, path);
                goto exit;
            case 'e': // [E]dit: let user manually enter a new title.
                ;
                char backup[MAX_TITLE_LEN];
                strcpy(backup, title);
                reset_terminal();
                printf("\nEnter new title: ");
                fgets(title, MAX_TITLE_LEN, stdin);
                title[strlen(title) - 1] = '\0'; // Remove Enter character.
                // Remove entered control characters.
                for (size_t i = 0; i < strlen(title); i++) {
                    if (iscntrl(title[i])) {
                        memmove(&title[i], &title[i + 1], strlen(title) - i);
                        i--;
                    }
                }
                // Restore title if nothing has been entered.
                if (title[0] == '\0') {
                    strcpy(title, backup);
                    printf("Using title \"%s\".\n", title);
                }
                printf("\n");
                raw_terminal();
                break;
            case 't': // [T]ag: let user enter release groups or releases.
                ;
                char tag[MAX_TAG_LEN] = "";
                printf("\nEnter new tag: ");
                scan_string(tag, MAX_TAG_LEN, "", get_tag);
                char *result;
                if (tag[0] != '\0') {
                    if ((result = get_release_group(tag)) != NULL) {
                        printf("Using \"%s\" as release group.\n", result);
                        strncpy(tag_release_group, result, MAX_TAG_LEN);
                        tag_release_group[MAX_TAG_LEN] = '\0';
                    } else if ((result = get_uploader(tag)) != NULL) {
                        printf("Using \"%s\" as release.\n", result);
                        strncpy(tag_release, result, MAX_TAG_LEN);
                        tag_release[MAX_TAG_LEN] = '\0';
                    } else if (strcmp(tag, "Backport") == 0) {
                        if (backport) {
                            printf("Backport tag disabled.\n");
                            backport = NULL;
                        } else {
                            printf("Backport tag enabled.\n");
                            backport = "Backport";
                        }
                    } else {
                        printf("Using \"%s\" as release.\n", tag);
                        strncpy(tag_release, tag, MAX_TAG_LEN);
                        tag_release[MAX_TAG_LEN] = '\0';
                    }
                }
                printf("\n");
                break;
            case 'm': // [M]ix: convert title to mixed-case letter format.
                mixed_case(title);
                printf("\nConverted letter case to mixed-case style.\n\n");
                break;
            case 'o': // [O]nline: search the PlayStation store for metadata.
                printf("\n");
                search_online(content_id, title, 0);
                printf("\n");
                break;
            case 'r': // [R]eset: undo all changes.
                strcpy(title, title_backup);
                printf("\nTitle has been reset to \"%s\".\n", title_backup);
                if (tag_release_group[0] != '\0') {
                    printf("Tagged release group \"%s\" has been reset.\n",
                        tag_release_group);
                    tag_release_group[0] = '\0';
                }
                if (tag_release[0] != '\0') {
                    printf("Tagged release \"%s\" has been reset.\n",
                        tag_release);
                    tag_release[0] = '\0';
                }
                printf("\n");
                break;
            case 'c': // [C]hars: reveal all non-printable characters.
                printf("\nOriginal: \"%s\"\nRevealed: \"", title);
                int count = 0;
                for (size_t i = 0; i < strlen(title); i++) {
                    if (isprint(title[i])) {
                        printf("%c", title[i]);
                    } else {
                        count++;
                        if (option_disable_colors == 0)
                            fputs(BRIGHT_YELLOW, stdout);
                        printf("#%u", (unsigned char) title[i]);
                        if (option_disable_colors == 0)
                            fputs(RESET, stdout);
                    }
                }
                printf("\"\n%d special character%c found.\n\n", count,
                count == 1 ? 0 : 's');
                break;
            case 's': // [S]FO: print param.sfo information.
                printf("\n");
                print_param_sfo(param_sfo);
                printf("\n");
                break;
            case 'h': // [H]elp: show help.
                printf("\n");
                print_prompt_help();
                printf("\n");
                break;
            case 'b': // [B]ackport: toggle backport tag.
                if (backport) {
                    backport = NULL;
                    printf("\nBackport tag disabled.\n\n");
                } else {
                    backport = "Backport";
                    printf("\nBackport tag enabled.\n\n");
                }
                break;
            case 'l': // Change[l]og: print existing changelog data.
                if (changelog) {
                    printf("\n%s\n\n", changelog);
                    print_changelog_tags(changelog);
                    putchar('\n');
                } else {
                    printf("\nThis file does not contain changelog data.\n\n");
                }
                break;
            case 'p': // [P]atch: toggle changelog patch detection for app PKGs.
                if (changelog_patch_detection) {
                    merged_ver = NULL;
                    true_ver = app_ver;
                    changelog_patch_detection = 0;
                    printf("\nChangelog patch detection disabled for the"
                        " current file.\n\n");
                } else {
                    if (true_ver_buf[0]) {
                        if (option_leading_zeros == 0 && true_ver_buf[0] == '0')
                            true_ver = true_ver_buf + 1;
                        else
                            true_ver = true_ver_buf;
                        if (category && category[1] == 'd'
                            && strcmp(true_ver_buf, "01.00") != 0)
                            merged_ver = true_ver;
                    }
                    changelog_patch_detection = 1;
                    printf("\nChangelog patch detection enabled for the current"
                        " file.\n\n");
                }
                break;
            case 'q': // [Q]uit: exit the program.
                free(old_path);
                exit(EXIT_SUCCESS);
        }
    }

exit:
    return 0;
}

// Background thread that scans PS4 PKG files for data required for renaming.
// The data is stored in a global linked list.
static void *scan_files(void *param)
{
    (void) param;

    if (noptc == 0) { // Use current directory.
        if (parse_directory("."))
            return NULL;
    } else { // Find PKGs and run pkgrename() on them.
        DIR *dir;
        for (int i = 0; i < noptc; i++) {
            // Directory
            if ((dir = opendir(nopts[i])) != NULL) {
                closedir(dir);
                if (parse_directory(nopts[i]))
                    return NULL;
            // File
            } else {
                struct scan_result *scan = scan_file(nopts[i], 0);
                if (scan)
                    add_scan_result(scan);
            }
        }
    }

    int err;

    if ((err = pthread_mutex_lock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);
    if (last_scan)
        last_scan->next = END_OF_SCAN;
    if ((err = pthread_mutex_unlock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);

    if ((err = pthread_cond_signal(&new_file_ready)) != 0)
        exit_err(err, __func__, __LINE__);

    return NULL;
}

// Runs pkgrename() on scan results as they become available.
static void parse_scan_results(void)
{
    int err;

start:
    if ((err = pthread_mutex_lock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);
    if ((err = pthread_cond_wait(&new_file_ready, &mutex)) != 0)
        exit_err(err, __func__, __LINE__);
    if ((err = pthread_mutex_unlock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);

    struct scan_result *scan = first_scan;
    if (scan == NULL || scan == END_OF_SCAN)
        return;

    int option_force_backup = option_force;
    while (1) {
        // Reset scan_backup if it's reached again without pressing space.
        if (scan == scan_backup)
            scan_backup = NULL;

        int ret = pkgrename(scan);

        // Go back to the previous scan result if requested.
        if (ret == 1) {
            if (scan_backup == NULL)
                scan_backup = scan;
            scan = scan->prev; // pkgrename() made sure it's not NULL.
            option_force = 1;
            continue;
        }

        // Return to the current scan result if requested.
        if (ret == 2) {
            scan = scan_backup;
            scan_backup = NULL;
            option_force = option_force_backup;
            continue;
        }

        // Delete the current node due to file error.
        if (ret == -1) {
            struct scan_result *prev_scan = scan->prev;
            delete_scan_result(scan);
            if (prev_scan == NULL)
                goto start;
            else
                scan = prev_scan;
        }

        // Proceed with the next scan result...
        if ((err = pthread_mutex_lock(&mutex)) != 0)
            exit_err(err, __func__, __LINE__);
        if (scan->next == NULL) {
            if ((err = pthread_cond_wait(&new_file_ready, &mutex)) != 0)
                exit_err(err, __func__, __LINE__);
        }
        if (scan->next == END_OF_SCAN) { // ...until there are no results left.
            if ((err = pthread_mutex_unlock(&mutex)) != 0)
                exit_err(err, __func__, __LINE__);
            return;
        }
        scan = scan->next;
        if ((err = pthread_mutex_unlock(&mutex)) != 0)
            exit_err(err, __func__, __LINE__);
    }
}

int main(int argc, char *argv[])
{
    initialize_terminal();
    raw_terminal();

#ifdef _WIN32
    // Disable colors if the terminal is not Windows Terminal.
    if (getenv("WT_SESSION") == NULL)
        option_disable_colors = 1;
#endif

    parse_options(argc, argv);

    // Run file scans in a separate thread.
    pthread_t file_thread;
    int err;
    if ((err = pthread_create(&file_thread, NULL, scan_files, NULL)) != 0)
        exit_err(err, __func__, __LINE__);

    // Parse the scan results in the main thread.
    parse_scan_results();

    // Debug
    //print_linked_list();

    exit(EXIT_SUCCESS);
}
