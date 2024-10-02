// Copyright (c) 2024 hippie68

// Small library to generate file/buffer checksums in an easy way.

#define _FILE_OFFSET_BITS 64

#include <openssl/evp.h>

// Note: "f" can be one of many functions provided by openssl.
// For example: EVP_md5, EVP_sha1, ...
// Returned checksum buffers must be freed with "OPENSSL_free()".

// Prints a checksum stored in a buffer to the provided file descriptor.
// Note: The buffer's and f's checksum types must match!
void print_checksum(FILE *stream, unsigned char *buf, const EVP_MD *(*f)(void));

// Calculates a file's checksum.
unsigned char *checksum_file(const EVP_MD *(*f)(void), char *filename, uint64_t start, uint64_t end);

// Calculates a buffer's checksum.
unsigned char *checksum_buf(const EVP_MD *(*f)(void), unsigned char *buf, size_t bufsize);
