// Copyright (c) 2024 hippie68

#include "../include/checksums.h"
#include <stdio.h>

#define FILE_BUF_SIZE 65536

void print_checksum(FILE *stream, unsigned char *buf, const EVP_MD *(*f)(void))
{
    for (int i = 0; i < EVP_MD_size(f()); i++)
        fprintf(stream, "%02x", buf[i]);
}

static int digest_update_file(EVP_MD_CTX *mdctx, char *filename, uint64_t start, uint64_t end)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        return -1;

    uint64_t remaining;
    if (end) {
        remaining = end - start;
    } else {
        if (fseek(file, 0, SEEK_END))
            return -1;
        remaining = ftell(file);
        if (fseek(file, start, SEEK_SET))
            return -1;
    }

    unsigned char buf[FILE_BUF_SIZE];
    while (remaining) {
        size_t n = fread(buf, 1, FILE_BUF_SIZE, file);
        if (n == 0) {
            if (feof(file))
                break;
            else {
                fclose(file);
                return -1;
            }
        }

        if (remaining < n)
            n = remaining;
        remaining -= n;

        EVP_DigestUpdate(mdctx, buf, n);
    }

    fclose(file);
    return 0;
}

unsigned char *checksum_file(const EVP_MD *(*f)(void), char *filename, uint64_t start, uint64_t end)
{
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, f(), NULL);

    if (digest_update_file(mdctx, filename, start, end))
        return NULL;

    unsigned int size = EVP_MD_size(f());
    unsigned char *checksum = OPENSSL_malloc(size);
    EVP_DigestFinal_ex(mdctx, checksum, &size);
    EVP_MD_CTX_free(mdctx);

    return checksum;
}

unsigned char *checksum_buf(const EVP_MD *(*f)(void), unsigned char *buf, size_t bufsize)
{
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, f(), NULL);

    EVP_DigestUpdate(mdctx, buf, bufsize);

    unsigned int size = EVP_MD_size(f());
    unsigned char *checksum = OPENSSL_malloc(size);
    EVP_DigestFinal_ex(mdctx, checksum, &size);
    EVP_MD_CTX_free(mdctx);

    return checksum;
}
