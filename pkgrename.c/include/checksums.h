#ifndef CHECKSUMS_H
#define CHECKSUMS_H

#include <stdio.h>

// Prints a checksum stored in a buffer to the provided file descriptor.
void print_checksum(FILE *stream, unsigned char *buf, size_t buf_size);

// Calculates a buffer's checksum.
void sha256(void *to, void *from, size_t from_size);

#endif
