#include "../include/checksums.h"
#include "../include/common.h"
#include "../include/pkg.h"
#include "../include/scan.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_NUMBER_PKG 0x7f434e54
#define MAGIC_NUMBER_PARAM_SFO 0x46535000
#define MAX_SIZE_PARAM_SFO 65536
#define MAX_SIZE_CHANGELOG 65536

struct pkg_header {
    uint32_t magic;
    uint32_t type;
    uint32_t unknown_data;
    uint32_t file_count;
    uint32_t entry_count;
    uint32_t garbage_data;
    uint32_t table_offset;
    uint32_t entry_data_size;
    uint64_t body_offset;
    uint64_t body_size;
    uint64_t content_offset;
    uint64_t content_size;
    unsigned char content_id[36];
    unsigned char padding[12];
    uint32_t drm_type;
    uint32_t content_type;
    uint32_t content_flags;
} __attribute__ ((packed, scalar_storage_order("big-endian"))); // Requires GCC.

struct pkg_table_entry {
    uint32_t id;
    uint32_t filename_offset;
    uint32_t flags1;
    uint32_t flags2;
    uint32_t offset;
    uint32_t size;
    uint64_t padding;
} __attribute__ ((packed, scalar_storage_order("big-endian")));

struct param_sfo_header {
    uint32_t magic;
    uint32_t version;
    uint32_t keytable_offset;
    uint32_t datatable_offset;
    uint32_t n_entries;
} __attribute__ ((packed, scalar_storage_order("little-endian")));

struct param_sfo_entry {
    uint16_t key_offset;
    uint16_t param_type;
    uint32_t param_len;
    uint32_t param_max_len;
    uint32_t data_offset;
} __attribute__ ((packed, scalar_storage_order("little-endian")));

// Checks a buffered param.sfo file's integrity.
// Returns 0 on success or -1 on failure.
static int check_param_sfo(const unsigned char *param_sfo_buf, size_t buf_size)
{
    if (buf_size < sizeof(struct param_sfo_header))
        return -1;

    struct param_sfo_header *header = (struct param_sfo_header *) param_sfo_buf;
    if (buf_size < sizeof(struct param_sfo_header) + header->n_entries
        * sizeof(struct param_sfo_entry)
        || header->keytable_offset >= buf_size
        || header->datatable_offset >= buf_size)
        return -1;

    struct param_sfo_entry *entries = (struct param_sfo_entry *)
        &param_sfo_buf[sizeof(struct param_sfo_header)];
    for (uint32_t i = 0; i < header->n_entries; i++) {
        if (header->keytable_offset + entries[i].key_offset >= buf_size
            || header->datatable_offset + entries[i].data_offset
                + entries[i].param_len >= buf_size)
            return -1;
    }

    return 0;
}

static unsigned char *gen_key(char *content_id, char *passcode,
    int index)
{
    unsigned char index_buf[4] = { 0, 0, 0, index };
    unsigned char content_id_buf[48] = { 0 };
    memcpy(content_id_buf, content_id, strlen(content_id));

    unsigned char *checksum1 = checksum_buf(EVP_sha256, index_buf, 4);
    /* print_checksum(stdout, checksum1, EVP_sha256); */
    /* printf("\n"); */
    unsigned char *checksum2 = checksum_buf(EVP_sha256, content_id_buf, 48);
    /* print_checksum(stdout, checksum2, EVP_sha256); */
    /* printf("\n"); */

    unsigned char data[96];
    memcpy(data, checksum1, 32);
    memcpy(data + 32, checksum2, 32);
    memcpy(data + 64, passcode, 32);

    OPENSSL_free(checksum1);
    OPENSSL_free(checksum2);

    return checksum_buf(EVP_sha256, data, 96);
}

static _Bool is_fake(char *content_id, unsigned char key_checksum[32])
{
    static char *passcode = "00000000000000000000000000000000";

    unsigned char *key = gen_key(content_id, passcode, 0);
    unsigned char *checksum = checksum_buf(EVP_sha256, key, 32);
    for (int i = 0; i < 32; i++)
        checksum[i] = key[i] ^ checksum[i];
    OPENSSL_free(key);

    // Debug
    /* print_checksum(stdout, key_checksum, EVP_sha256); */
    /* print_checksum(stdout, checksum, EVP_sha256); */
    /* printf("\n"); */

    int ret = memcmp(checksum, key_checksum, 32);

    OPENSSL_free(checksum);

    if (ret)
        return false;
    return true;
}

// Loads PKG data into dynamically allocated buffers and passes their pointers.
// Returns 0 on success and -1 on error.
int load_pkg_data(unsigned char **param_sfo, char **changelog,
    _Bool *fake_status, const char *filename)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wscalar-storage-order"
    int retval;

    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        return SCAN_ERROR_OPEN_FILE;

    struct pkg_header pkg_header;
    if (fread(&pkg_header, sizeof(pkg_header), 1, file) != 1)
        goto read_error;
    if (pkg_header.magic != MAGIC_NUMBER_PKG) {
        retval = SCAN_ERROR_NOT_A_PKG;
        goto error;
    }

    // Get offsets and file sizes first, not to drop read-ahead cache.
    uint32_t param_sfo_offset, param_sfo_size, changelog_offset, keys_offset,
        changelog_size;
    int keys_offset_found = 0;
    int param_sfo_found = 0;
    int changelog_found = 0;
    if (fseek(file, pkg_header.table_offset, SEEK_SET))
        goto read_error;
    for (uint32_t i = 0; i < pkg_header.entry_count; i++) {
        struct pkg_table_entry entry;
        if (fread(&entry, sizeof(entry), 1, file) != 1)
            goto read_error;

        if (entry.id == 0x10) {
            keys_offset = entry.offset;
            keys_offset_found = 1;
        } else if (entry.id == 0x1000) { // param.sfo
            param_sfo_offset = entry.offset;
            if (entry.size > MAX_SIZE_PARAM_SFO) {
                retval = SCAN_ERROR_PARAM_SFO_INVALID_SIZE;
                goto error;
            }
            param_sfo_size = entry.size;
            param_sfo_found = 1;
        } else if (entry.id == 0x1260) { // changeinfo.xml
            changelog_offset = entry.offset;
            if (entry.size > MAX_SIZE_CHANGELOG) {
                retval = SCAN_ERROR_CHANGELOG_INVALID_SIZE;
                goto error;
            }
            changelog_size = entry.size;
            changelog_found = 1;
        }
    }

    // Load param.sfo.
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    if (param_sfo_found) {
        if (fseek(file, param_sfo_offset, SEEK_SET))
            goto read_error;

        unsigned char *param_sfo_buf = malloc(param_sfo_size + 1);
        if (param_sfo_buf == NULL) {
            retval = SCAN_ERROR_OUT_OF_MEMORY;
            goto read_error;
        }

        if (fread(param_sfo_buf, param_sfo_size, 1, file) != 1)
            goto read_error;

        if ((*(struct param_sfo_header *) param_sfo_buf).magic
            != MAGIC_NUMBER_PARAM_SFO) {
            retval = SCAN_ERROR_PARAM_SFO_INVALID_FORMAT;
            goto error;
        }

        if (check_param_sfo(param_sfo_buf, param_sfo_size)) {
            retval = SCAN_ERROR_PARAM_SFO_INVALID_DATA;
            goto error;
        }

        // Guard against non-terminated keytable.
        param_sfo_buf[param_sfo_size] = '\0';

        *param_sfo = param_sfo_buf;
    } else {
        retval = SCAN_ERROR_PARAM_SFO_NOT_FOUND;
        goto error;
    }

    // Load changelog.
    if (changelog_found) {
        if (fseek(file, changelog_offset, SEEK_SET))
            goto read_error;

        char *changelog_buf = malloc(changelog_size + 1);
        if (changelog_buf == NULL) {
            retval = SCAN_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        if (fread(changelog_buf, changelog_size, 1, file) != 1)
            goto read_error;
        changelog_buf[changelog_size] = '\0'; // Make it a C string.

        *changelog = changelog_buf;
    } else {
        *changelog = NULL;
    }

    // Check for FPKG.
    if (keys_offset_found) {
        if (fseek(file, keys_offset + 32, SEEK_SET))
            goto read_error;

        unsigned char key_checksum[32];
        if (fread(key_checksum, 32, 1, file) != 1)
            goto read_error;

        char *content_id = get_param_sfo_value(*param_sfo, "CONTENT_ID");
        if (content_id == NULL) {
            retval = SCAN_ERROR_PARAM_SFO_INVALID_DATA;
            goto error;
        }

        *fake_status = is_fake(content_id, key_checksum);
    }

    fclose(file);
    return 0;

read_error:
    retval = SCAN_ERROR_READ_FILE;
error:
    fclose(file);
    return retval;
#pragma GCC diagnostic pop
}

// Loads the true patch version from a string and stores it in a buffer;
// the buffer must be of size 6.
// Returns 1 if the patch version has been found, otherwise 0.
int store_patch_version(char *version_buf, const char *changelog)
{
    // Grab the highest patch version.
    const char *bufp = changelog;
    char *next_patch;
    char current_patch[6];
    while ((next_patch = strstr(bufp, "app_ver=\"")) != NULL) {
        next_patch += 9;
        if (version_buf[0] == '\0') {
            strncpy(version_buf, next_patch, 5);
            version_buf[5] = '\0';
        } else {
            strncpy(current_patch, next_patch, 5);
            current_patch[5] = '\0';
            if (strcmp(version_buf, current_patch) < 0)
                memcpy(version_buf, current_patch, 6);
        }

        bufp = next_patch;
    }

    return version_buf[0] != '\0';
}

// Searches a buffered param.sfo file for a key/value pair and returns a pointer
// to the value. Returns NULL if the key is not found.
void *get_param_sfo_value(const unsigned char *param_sfo_buf, const char *key)
{
    struct param_sfo_header *header = (struct param_sfo_header *) param_sfo_buf;
    struct param_sfo_entry *entries = (struct param_sfo_entry *)
        &param_sfo_buf[sizeof(struct param_sfo_header)];

    for (uint32_t i = 0; i < header->n_entries; i++) {
        char *current_key = (char *) &param_sfo_buf[header->keytable_offset
            + entries[i].key_offset];
        if (strcmp(current_key, key) == 0)
            return (void *) &param_sfo_buf[header->datatable_offset
                + entries[i].data_offset];
    }

    return NULL;
}

// Prints a buffered param.sfo file's keys and values.
void print_param_sfo(const unsigned char *param_sfo_buf)
{
    struct param_sfo_header *header = (struct param_sfo_header *) param_sfo_buf;
    struct param_sfo_entry *entries = (struct param_sfo_entry *)
        &param_sfo_buf[sizeof(struct param_sfo_header)];

    for (uint32_t i = 0; i < header->n_entries; i++) {
        char *key = (char *) &param_sfo_buf[header->keytable_offset
            + entries[i].key_offset];
        void *val = (void *) &param_sfo_buf[header->datatable_offset
            + entries[i].data_offset];

        switch (entries[i].param_type) {
            case 0x0004:
            case 0x0204:
                printf("%s=\"%s\"\n", key, (char *) val);
                break;
            case 0x0404:
                printf("%s=0x%08X\n", key, *(uint32_t *) val);
                break;
        }
    }
}

int get_checksum(char msum[7], const char *filename)
{
    unsigned char buf[32];

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", filename);
        return -1;
    }

    struct pkg_header header;
    if (fread((void *) &header, sizeof(header), 1, file) != 1)
        goto read_error;

    if (header.content_type == 27) // DLC
        goto error;

    uint32_t target_id;
    switch (header.content_flags & 0x0F000000) {
        case 0x0A000000:
            target_id = 0x1001;
            break;
        case 0x02000000:
            target_id = 0x1008;
            break;
        default:
            goto error;
    }

    if (fseek(file, header.table_offset, SEEK_SET))
        goto read_error;
    struct pkg_table_entry entry;
    if (fread((void *) &entry, sizeof(entry), 1, file) != 1)
        goto read_error;
    uint32_t digests_offset = entry.offset;
    for (uint32_t i = 1; i < header.entry_count; i++) {
        if (fread((void *) &entry, sizeof(entry), 1, file) != 1)
            goto read_error;
        if (entry.id == target_id) {
            if (fseek(file, digests_offset + i * 32, SEEK_SET))
                goto read_error;
            if (fread(buf, 32, 1, file) != 1)
                goto read_error;

            fclose(file);
            for (int c = 0; c < 3; c++)
                sprintf(msum + c * 2, "%02X", buf[c]);
            msum[6] = '\0';
            return 0;
        }
    }
    goto error;

read_error:
    fprintf(stderr, "Could not read from file \"%s\".\n", filename);
error:
    fclose(file);
    return -1;
}
