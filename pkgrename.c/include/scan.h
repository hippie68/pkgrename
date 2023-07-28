#ifndef SCAN_H
#define SCAN_H

#include <pthread.h>

struct scan_result {
    char *filename;
    _Bool filename_is_allocated;
    unsigned char *param_sfo;
    char *changelog;
    enum {
        SCAN_ERROR_OPEN_FILE = 1,
        SCAN_ERROR_READ_FILE,
        SCAN_ERROR_NOT_A_PKG,
        SCAN_ERROR_OUT_OF_MEMORY,
        SCAN_ERROR_PARAM_SFO_INVALID_DATA,
        SCAN_ERROR_PARAM_SFO_INVALID_FORMAT,
        SCAN_ERROR_PARAM_SFO_INVALID_SIZE,
        SCAN_ERROR_PARAM_SFO_NOT_FOUND,
        SCAN_ERROR_CHANGELOG_INVALID_SIZE,
    } error;
    struct scan_result *next;
    struct scan_result *prev;
};

extern pthread_mutex_t mutex;
extern pthread_cond_t new_file_ready;
extern struct scan_result end_of_scan;
extern struct scan_result *END_OF_SCAN; // The .next member that is END_OF_SCAN
                                        // indicates scanning is done.
extern struct scan_result *first_scan, *last_scan;

// Adds an allocated scan result to the global linked list.
void add_scan_result(struct scan_result *scan);

// Deletes a single scan result from the global linked list and destroys it.
void delete_scan_result(struct scan_result *scan);

// Destroys all scan results found the global linked list.
// Supposed to only run after threads have terminated.
void destroy_scan_results(void);

// Scans a file for PS4 PKG content and returns the allocated scan result
// on success or NULL on out of memory error.
// If the file is dynamically allocated, the integer parameter must be true.
struct scan_result *scan_file(char *filename, _Bool filename_is_allocated);

// Prints an error message that describes the integer value of struct
// scan_result's .error member.
void print_scan_error(struct scan_result *scan);

// Finds all .pkg files in a directory and runs a scan on them.
int parse_directory(char *directory_name);

#endif
