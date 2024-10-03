#include "../include/checksums.h"
#include "../include/sha256.h"

void print_checksum(FILE *stream, unsigned char *buf, size_t buf_size)
{
    for (size_t i = 0; i < buf_size; i++)
        fprintf(stream, "%02x", buf[i]);
}

void sha256(void *to, void *from, size_t from_size)
{
    Sha256Context context;
    Sha256Initialise(&context);
    Sha256Update(&context, from, from_size);
    Sha256Finalise(&context, to);
}
