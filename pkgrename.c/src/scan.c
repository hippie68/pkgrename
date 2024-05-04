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

#define SCAN_LIST_MIN_SIZE 1024

#ifdef DEBUG
// Debug function.
void print_scan(struct scan *scan)
{
    fprintf(stderr, "node pointer: %p\n", (void *) scan );
    if (scan == NULL)
        return;
    fprintf(stderr, "filename: %s\n", scan->filename);
    fprintf(stderr, "filename_allocated: %d\n", scan->filename_allocated);
    fprintf(stderr, "prev: %p\n", (void *) scan->prev);
    fprintf(stderr, "next: %p\n", (void *) scan->next);
}
#endif

// Companion function for initialize_scan_job().
// Returns 0 on success and -1 on error.
static inline int initialize_scan_list(struct scan_list *list)
{
    list->tail = calloc(1, SCAN_LIST_CHUNK_SIZE * sizeof(struct scan));
    if (list->tail == NULL)
        return -1;

    list->head = NULL;
    list->finished = 0;
    list->n_slots = SCAN_LIST_CHUNK_SIZE;

    return 0;
}

// Initializes a scan job.
// Returns 0 on success and -1 on error.
int initialize_scan_job(struct scan_job *job, char **filenames, int n_filenames)
{
    if (initialize_scan_list(&job->scan_list))
        return -1;
    if (pthread_mutex_init(&job->mutex, NULL))
        return -1;
    if (pthread_cond_init(&job->cond, NULL)) {
        pthread_mutex_destroy(&job->mutex);
        return -1;
    }
    job->filenames = filenames;
    job->n_filenames = n_filenames;

    return 0;
}

// Companion function for destroy_scan_list(); must not be used elsewhere.
// Deletes a node and returns a pointer to the next node.
static inline struct scan *delete_scan(struct scan *scan)
{
    if (scan->filename_allocated && scan->filename)
        free(scan->filename);
    if (scan->param_sfo)
        free(scan->param_sfo);
    if (scan->changelog)
        free(scan->changelog);

    if (scan->next)
        return scan->next;
    else
        return NULL;
}

// Companion function for destroy_scan_job() that frees a scan list's allocated
// memory. The list must not be in use by other threads anymore.
static inline void destroy_scan_list(struct scan_list *scan_list)
{
    struct scan *scan = scan_list->head;
    if (scan == NULL)
        return;

    size_t n_nodes = 0;
    do {
        scan = delete_scan(scan);

        n_nodes++;
        if (n_nodes == SCAN_LIST_CHUNK_SIZE) {
            free(scan_list->head);
            scan_list->head = scan;
            n_nodes = 0;
        }
    } while (scan);
}

// Destroys a scan job.
void destroy_scan_job(struct scan_job *job)
{
    destroy_scan_list(&job->scan_list);
    pthread_mutex_destroy(&job->mutex);
    pthread_cond_destroy(&job->cond);
}

// Adds a scan result to a job's scan list.
void add_scan_result(struct scan_job *job, char *filename,
    _Bool filename_allocated)
{
    pthread_mutex_lock(&job->mutex);

    struct scan_list *list = &job->scan_list;
    struct scan *scan;

    // Assemble new node, without linking it yet.
    if (list->head == NULL) {
        scan = list->tail;
    } else {
        if (list->n_slots > 0) { // Chunk slots left? Use next slot.
            scan = list->tail + 1;
        } else { // Use a new chunk.
            scan = malloc(SCAN_LIST_CHUNK_SIZE * sizeof(struct scan));
            if (scan == NULL)
                exit(EXIT_FAILURE); // TODO: better error handling.
            list->n_slots = SCAN_LIST_CHUNK_SIZE;
        }
    }
    list->n_slots--;

    pthread_mutex_unlock(&job->mutex);

    scan->filename = filename;
    scan->filename_allocated = filename_allocated;
    scan->param_sfo = NULL;
    scan->changelog = NULL;
    scan->error = load_pkg_data(&scan->param_sfo, &scan->changelog, filename);
    scan->next = NULL;

    // Link new node.
    pthread_mutex_lock(&job->mutex);
    if (list->head == NULL) {
        list->tail = scan;
        list->head = scan;
    } else {
        list->tail->next = scan;
        scan->prev = list->tail;
        list->tail = scan;
    }

    pthread_mutex_unlock(&job->mutex);

    pthread_cond_signal(&job->cond);
}

// Prints a message that describes the value of struct scan's .error member.
void print_scan_error(struct scan *scan)
{
    set_color(BRIGHT_RED, stderr);
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
    set_color(RESET, stderr);
}

// Companion function for qsort in parse_directory().
static int qsort_compare_strings(const void *p, const void *q)
{
    return strcmp(*(const char **)p, *(const char **)q);
}

inline static int is_root(const char *dir)
{
    if (dir[0] == DIR_SEPARATOR && dir[1] == '\0')
        return 1;
    return 0;
}

// Finds all .pkg files in a directory and runs a scan on them.
// Returns 0 on success and -1 on error.
int parse_directory(char *cur_dir, struct scan_job *job)
{
    int retval = 0;

    DIR *dir;
    struct dirent *dir_entry;

    size_t dir_count_max = 100;
    size_t file_count_max = 100;
    char **dir_names = malloc(sizeof(void *) * dir_count_max);
    char **filenames = malloc(sizeof(void *) * file_count_max);
    size_t dir_count = 0, file_count = 0;

    // Remove trailing directory separators.
    {
        int len = strlen(cur_dir);
        while (len > 1 && cur_dir[len - 1] == DIR_SEPARATOR) {
            cur_dir[len - 1] = '\0';
            len--;
        }
    }

    dir = opendir(cur_dir); // TODO: better error handling.
    if (dir == NULL) {
        retval = -1;
        goto cleanup;
    }

    // Read all directory entries to put them in lists.
    while ((dir_entry = readdir(dir)) != NULL) {
        // Entry is a directory.
#ifdef _WIN32 // MinGW does not know .d_type.
        struct stat statbuf;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s%c%s", is_root(cur_dir) ? "" : cur_dir,
            DIR_SEPARATOR, dir_entry->d_name);
        if (stat(path, &statbuf) == -1) {
            set_color(BRIGHT_RED, stderr);
            fprintf(stderr, "Could not read file system information: \"%s\".\n",
                path);
            set_color(RESET, stderr);
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)) {
#else
        if (dir_entry->d_type == DT_DIR) {
#endif
            // Save name in the directory list.
            if (option_recursive == 1
                && dir_entry->d_name[0] != '.'
                && dir_entry->d_name[0] != '$') // Exclude system dirs.
            {
                size_t size;
                if (is_root(cur_dir))
                    size = 1 + strlen(dir_entry->d_name) + 1;
                else
                    size = strlen(cur_dir) + 1 + strlen(dir_entry->d_name) + 1;
                if ((dir_names[dir_count] = malloc(size)) == NULL) {
                    retval = -1;
                    goto cleanup;
                }

                sprintf(dir_names[dir_count], "%s%c%s", is_root(cur_dir) ? ""
                    : cur_dir, DIR_SEPARATOR, dir_entry->d_name);
                dir_count++;
                if (dir_count == dir_count_max) {
                    dir_count_max *= 2;
                    dir_names = realloc(dir_names, sizeof(void *)
                        * dir_count_max);
                }
            }
        // Entry is .pkg file.
        } else {
            char *file_extension = strrchr(dir_entry->d_name, '.');
            if (file_extension != NULL
                && strcasecmp(file_extension, ".pkg") == 0)
            {
                // Save name in the file list.
                size_t size;
                if (is_root(cur_dir))
                    size = 1 + strlen(dir_entry->d_name) + 1;
                else
                    size = strlen(cur_dir) + 1 + strlen(dir_entry->d_name) + 1;
                if ((filenames[file_count] = malloc(size)) == NULL) {
                    retval = -1;
                    goto cleanup;
                }

                sprintf(filenames[file_count], "%s%c%s", is_root(cur_dir) ?  ""
                    : cur_dir, DIR_SEPARATOR,  dir_entry->d_name);
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
    qsort(dir_names, dir_count, sizeof(char *),
        qsort_compare_strings);
    qsort(filenames, file_count, sizeof(char *), qsort_compare_strings);

    // Use the filenames to create new scan results.
    for (size_t i = 0; i < file_count; i++)
        add_scan_result(job, filenames[i], 1);

    // Parse sorted directories recursively.
    if (option_recursive == 1) {
        for (size_t i = 0; i < dir_count; i++) {
            parse_directory(dir_names[i], job);
            free(dir_names[i]);
        }
    }

    closedir(dir);

cleanup:
    free(dir_names);
    free(filenames);

    return retval;
}
