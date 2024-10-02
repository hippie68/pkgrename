#ifndef PKG_H
#define PKG_H

// Loads PKG data into dynamically allocated buffers and passes their pointers.
// Returns 0 on success and -1 on error.
int load_pkg_data(unsigned char **param_sfo, char **changelog,
    _Bool *fake_status, const char *filename);

// Searches a buffered param.sfo file for a key/value pair and returns a pointer
// to the value. Returns NULL if the key is not found.
void *get_param_sfo_value(const unsigned char *param_sfo_buf, const char *key);

// Prints a buffered param.sfo file's keys and values.
void print_param_sfo(const unsigned char *param_sfo_buf);

// Loads the true patch version from a string and stores it in a buffer;
// the buffer must be of size 6.
// Returns 1 if the patch version has been found, otherwise 0.
int store_patch_version(char *version_buf, const char *changelog);

// Get a PKG file's compatibility checksum.
int get_checksum(char msum[7], const char *filename);

#endif
