#include <stdint.h>

// Stores a single SFO parameter's data
struct sfo_parameter {
  char *name;
  int type;
  union {
    uint32_t *integer;
    char *string;
  };
};

// Reads all SFO parameters' data from a file, stores it in memory via malloc()
// and returns a pointer to the first structure.
// "count" returns the total number of stuctures.
// Access individual parameters via subscripting ("...[0], ...[1], ...[n]").
struct sfo_parameter *sfo_read(int *count, const char *filename);

// Prints a PKG file's param.sfo data.
void print_sfo(const char *filename);

// Loads a PKG file's changelog into a buffer.
// Returns -1 on failure, 0 on success.
int load_changelog(char *buffer, int bufsize, const char *filename);

// Loads the true patch version from a string and stores it in a buffer;
// the buffer must be of size 6
// Returns -1 on read error, 1 if the patch version has been found, otherwise 0
int get_patch_version(char *version_buf, const char *changelog);
