#include "../include/colors.h"
#include "../include/common.h"
#include "../include/scan.h"
#include "../include/options.h"
#include "../include/pkg.h"

#ifdef _WIN32
#include <sys/stat.h>
#endif

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_file_ready = PTHREAD_COND_INITIALIZER;
struct scan_result end_of_scan;
struct scan_result *END_OF_SCAN = &end_of_scan;
struct scan_result *first_scan, *last_scan;

// Adds an allocated scan result to the global linked list.
void add_scan_result(struct scan_result *scan)
{
    int err;

    if ((err = pthread_mutex_lock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);

    if (first_scan == NULL) { // Also means last_scan == NULL.
        scan->prev = NULL;
        scan->next = NULL;
        first_scan = scan;
        last_scan = scan;
    } else {
        last_scan->next = scan;
        scan->prev = last_scan;
        scan->next = NULL;
        last_scan = scan;
    }

    if ((err = pthread_mutex_unlock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);
    if ((err = pthread_cond_signal(&new_file_ready)) != 0)
        exit_err(err, __func__, __LINE__);

    return;
}

// Deletes a single scan result from the global linked list and destroys it.
void delete_scan_result(struct scan_result *scan)
{
    int err;

    if ((err = pthread_mutex_lock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);

    // Unlink node.
    if (scan->next && scan->next != END_OF_SCAN)
        scan->next->prev = scan->prev;
    if (scan->prev)
        scan->prev->next = scan->next;

    // Delete contents.
    if (scan->filename_is_allocated)
        free(scan->filename);
    free(scan->param_sfo);
    free(scan->changelog);

    // Set new head and tail.
    if (scan == first_scan)
        first_scan = scan->next;
    if (scan == last_scan)
        last_scan = scan->prev;

    // Delete node.
    free(scan);

    if ((err = pthread_mutex_unlock(&mutex)) != 0)
        exit_err(err, __func__, __LINE__);
}

// Destroys all scan results found in the global linked list.
// Supposed to only run after threads have terminated.
void destroy_scan_results(void)
{
    struct scan_result *scan = first_scan;

    while (scan != NULL && scan != END_OF_SCAN) {
        if (scan->filename_is_allocated)
            free(scan->filename);
        free(scan->param_sfo);
        free(scan->changelog);

        if (scan->next == NULL || scan->next == END_OF_SCAN) {
            scan = scan->next;
            free(scan->prev);
        } else {
            free(scan);
            break;
        }
    }
}

// Scans a file for PS4 PKG content and returns the allocated scan result
// on success or NULL on out of memory error.
// If the file is dynamically allocated, the integer parameter must be true.
struct scan_result *scan_file(char *filename, _Bool filename_is_allocated)
{
    struct scan_result *scan = malloc(sizeof *scan);
    if (scan == NULL) {
        fputs("Out of memory while scanning remaining files.\n", stderr);
        return NULL;
    }
    scan->filename = filename;
    scan->filename_is_allocated = filename_is_allocated;
    scan->param_sfo = NULL;
    scan->changelog = NULL;
    scan->error = load_pkg_data(&scan->param_sfo, &scan->changelog,
        scan->filename);
    // Note: .prev and .next are set by add_scan_result().

    return scan;
}

// Prints an error message that describes the integer value of struct
// scan_result's .error member.
void print_scan_error(struct scan_result *scan)
{
    if (option_disable_colors == 0)
        fputs(BRIGHT_RED, stderr);
    fprintf(stderr, "Error while scanning file \"%s\": ", scan->filename);
    switch (scan->error) {
        case SCAN_ERROR_NOT_A_PKG:
            fputs("File is not a PS4 PKG file.\n", stderr);
            break;
        case SCAN_ERROR_OPEN_FILE:
            fputs("Could not open file.\n", stderr);
            break;
        case SCAN_ERROR_READ_FILE:
            fputs("Could not read data.\n", stderr);
            break;
        case SCAN_ERROR_OUT_OF_MEMORY:
            fputs("Could not allocate memory for PKG content.\n", stderr);
            break;
        case SCAN_ERROR_PARAM_SFO_INVALID_DATA:
            fputs("Invalid data in PKG content \"param.sfo\".\n", stderr);
            break;
        case SCAN_ERROR_PARAM_SFO_INVALID_FORMAT:
            fputs("Invalid file type of PKG content \"param.sfo\".\n", stderr);
            break;
        case SCAN_ERROR_PARAM_SFO_INVALID_SIZE:
            fputs("Invalid size of PKG content \"param.sfo\".\n", stderr);
            break;
        case SCAN_ERROR_PARAM_SFO_NOT_FOUND:
            fputs("PKG content \"param.sfo\" not found.\n", stderr);
            break;
        case SCAN_ERROR_CHANGELOG_INVALID_SIZE:
            fputs("Invalid size of PKG content \"changelog.xml\".\n", stderr);
            break;
        default:
            fputs("Unkown error.\n", stderr);
    }
    if (option_disable_colors == 0)
        fputs(RESET, stderr);
}

// Companion function for qsort in parse_directory().
static int qsort_compare_strings(const void *p, const void *q) {
    return strcmp(*(const char **)p, *(const char **)q);
}

// Finds all .pkg files in a directory and runs a scan on them.
int parse_directory(char *directory_name)
{
    int retval = 0;

    DIR *dir;
    struct dirent *directory_entry;

    size_t directory_count_max = 100;
    size_t file_count_max = 100;
    char **directory_names = malloc(sizeof(void *) * directory_count_max);
    char **filenames = malloc(sizeof(void *) * file_count_max);
    size_t directory_count = 0, file_count = 0;

    // Remove a possibly trailing directory separator.
    {
        int len = strlen(directory_name);
        if (len > 1 && directory_name[len - 1] == DIR_SEPARATOR) {
            directory_name[len - 1] = '\0';
        }
    }

    dir = opendir(directory_name); // TODO: better error handling.
    if (dir == NULL) {
        retval = -1;
        goto cleanup;
    }

    // Read all directory entries to put them in lists.
    while ((directory_entry = readdir(dir)) != NULL) {
        // Entry is a directory.
#ifdef _WIN32 // MinGW does not know .d_type.
        struct stat statbuf;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s%c%s", directory_name, DIR_SEPARATOR,
            directory_entry->d_name);
        if (stat(path, &statbuf) == -1) {
            fprintf(stderr, "Could not read file information: \"%s\".\n", path);
            retval = -1;
            goto full_cleanup;
        }
        if (S_ISDIR(statbuf.st_mode)) {
#else
        if (directory_entry->d_type == DT_DIR) {
#endif
            // Save name in the directory list
            if (option_recursive == 1
                && directory_entry->d_name[0] != '.'
                && directory_entry->d_name[0] != '$') { // Exclude system dirs.
                directory_names[directory_count] = malloc(strlen(directory_name)
                    + 1 + strlen(directory_entry->d_name) + 1);
                sprintf(directory_names[directory_count], "%s%c%s",
                    directory_name, DIR_SEPARATOR, directory_entry->d_name);
                directory_count++;
                if (directory_count == directory_count_max) {
                    directory_count_max *= 2;
                    directory_names = realloc(directory_names,
                        sizeof(void *) * directory_count_max);
                }
            }
        // Entry is .pkg file.
        } else {
            char *file_extension = strrchr(directory_entry->d_name, '.');
            if (file_extension != NULL
                && strcasecmp(file_extension, ".pkg") == 0) {
                // Save name in the file list.
                filenames[file_count] = malloc(strlen(directory_name) + 1
                    + strlen(directory_entry->d_name) + 1);
                sprintf(filenames[file_count], "%s%c%s", directory_name,
                    DIR_SEPARATOR,  directory_entry->d_name);
                file_count++;
                if (file_count == file_count_max) {
                    file_count_max *= 2;
                    filenames = realloc(filenames, sizeof(void *)
                        * file_count_max);
                }
            }
        }
    }

    // Sort the final lists.
    qsort(directory_names, directory_count, sizeof(char *),
        qsort_compare_strings);
    qsort(filenames, file_count, sizeof(char *), qsort_compare_strings);

    // Use the lists to create new scan results.
    for (size_t i = 0; i < file_count; i++) {
        struct scan_result *scan;
        if ((scan = scan_file(filenames[i], 1)) == NULL) {
            retval = -1;
            goto full_cleanup;
        } else {
            add_scan_result(scan);
        }
    }

    // Parse sorted directories recursively.
    if (option_recursive == 1) {
        for (size_t i = 0; i < directory_count; i++) {
            parse_directory(directory_names[i]);
            free(directory_names[i]);
        }
    }

full_cleanup:
    closedir(dir);

cleanup:
    free(directory_names);
    free(filenames);

    return retval;
}
