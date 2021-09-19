#include <stdint.h>

#define PARAM_NAME_SIZE 50
#define PARAM_STRING_SIZE 512
#define TYPE_SPECIAL_MODE_STRING 0
#define TYPE_STRING 1
#define TYPE_INTEGER 2

// Stores a single SFO parameter's data
struct sfo_parameter {
  char name[PARAM_NAME_SIZE];
  int type;
  union {
    uint32_t integer;
    char string[PARAM_STRING_SIZE];
  };
};

// Reads all SFO parameters' data from a file, stores it in memory via malloc()
// and returns a pointer to the first structure.
// "count" represents the total number of stuctures.
// Access individual parameters via subscripting ("...[0], ...[1], ...[n]").
struct sfo_parameter *sfo_read(int *count, char *filename);

void print_sfo(char *filename);
