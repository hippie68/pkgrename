// Loads PKG data into dynamically allocated buffers.
// If NULL is passed for <changelog>, the PKG will not be searched for changelog
// data and the buffer will not be allocated.
// Returns 0 on success and -1 on error.
int load_pkg_data(unsigned char **param_sfo, char **changelog,
    const char *filename);

// Prints a buffered param.sfo file's keys and values.
void print_param_sfo(const unsigned char *param_sfo_buf);

// Loads the true patch version from a string and stores it in a buffer;
// the buffer must be of size 6.
// Returns -1 on read error, 1 if the patch version has been found, otherwise 0.
int get_patch_version(char *version_buf, const char *changelog);

// Searches a buffered param.sfo file for a key/value pair and returns a pointer
// to the value. Returns NULL if the key is not found.
void *get_param_sfo_value(const unsigned char *param_sfo_buf, const char *key);
