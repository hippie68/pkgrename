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
struct sfo_parameter *sfo_read(int *count, char *filename);

// Loads the true patch version from a PKG and stores it in a buffer;
// the buffer must be of size 6
int get_patch_version(char *version_buf, char *filename);

// Prints a PKG file's param.sfo data.
void print_sfo(char *filename);
