#ifndef SCAN_H
#define SCAN_H

#include <pthread.h>

// Linked list node that stores a PS4 PKG file scan result.
struct scan {
    char *filename;
    unsigned char *param_sfo;
    char *changelog;
    _Bool fake_status;
    _Bool filename_allocated;
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
    struct scan *prev;
    struct scan *next;
};

// This is a linked list that does not allocate individual nodes but instead
// larger memory chunks on demand when it runs out of node slots (see scan.c).
#define SCAN_LIST_CHUNK_SIZE 1024 // Number of nodes per chunk.
struct scan_list {
    struct scan *head;
    struct scan *tail;
    _Bool finished; // True when scanning is complete.
    short n_slots; // Number of remaining slots in the current chunk.
};

struct scan_job {
    struct scan_list scan_list;
    pthread_mutex_t mutex;
    pthread_cond_t cond; // "A new scan result is ready."
    char **filenames; // May contain both files and directories.
    int n_filenames;
};

// Adds a scan result to a job's scan list.
void add_scan_result(struct scan_job *job, char *filename,
    _Bool filename_allocated);

// Prints a message that describes the value of struct scan's .error member.
void print_scan_error(struct scan *scan);

// Finds all .pkg files in a directory and runs a scan on them.
// Returns 0 on success and -1 on error.
int parse_directory(char *directory_name, struct scan_job *job);

// Initializes a scan job.
// Returns 0 on success and -1 on error.
int initialize_scan_job(struct scan_job *job, char **filenames,
    int n_filenames);

// Destroys a scan job.
void destroy_scan_job(struct scan_job *job);

#ifdef DEBUG
// Debug function.
void print_scan(struct scan *scan);
#endif

#endif
