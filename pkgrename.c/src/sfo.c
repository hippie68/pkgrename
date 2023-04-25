/* Parses a PS4 PKG or param.sfo file for SFO parameters */

#ifndef _WIN32
#include <byteswap.h>
#endif
#include "../include/sfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_NUMBER_PKG 1414415231
#define MAGIC_NUMBER_PARAM_SFO 1179865088
#define TYPE_SPECIAL_MODE_STRING 0
#define TYPE_STRING 1
#define TYPE_INTEGER 2

// Global variables
static FILE *file;
static char param_sfo[65536];

#ifdef _WIN32
// Replacement function for byteswap.h's bswap_16
static inline uint16_t bswap_16(uint16_t val) {
  return (val >> 8) | (val << 8);
}

// Replacement function for byteswap.h's bswap_32
static inline uint32_t bswap_32(uint32_t val) {
  val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0x00FF00FF );
  return (val << 16) | (val >> 16);
}
#endif

// Returns a 4-byte integer, reading from specified offset
static inline uint32_t get_uint32(size_t offset) {
  uint32_t val;
  fseek(file, offset, SEEK_SET);
  fread(&val, 4, 1, file);
  return val;
}

struct pkg_table_entry {
  uint32_t id;
  uint32_t filename_offset;
  uint32_t flags1;
  uint32_t flags2;
  uint32_t offset;
  uint32_t size;
  uint64_t padding;
};

// Finds the param.sfo file inside a PS4 PKG file and loads it into memory
// Returns -1 on read error, 1 if the PKG does not contain a param.sfo file
static int load_param_sfo(void) {
  const int pkg_table_offset = bswap_32(get_uint32(0x018));
  const int pkg_file_count = bswap_32(get_uint32(0x010));
  struct pkg_table_entry pkg_table_entry;

  // Search PKG file until param.sfo is found
  if (fseek(file, pkg_table_offset, SEEK_SET))
    return -1;
  for (int i = 0; i < pkg_file_count; i++) {
    if (fread(&pkg_table_entry, sizeof(struct pkg_table_entry), 1, file) != 1)
      return -1;

    // Load param.sfo into memory
    if (pkg_table_entry.id == bswap_32(0x00001000)) { // param.sfo file ID
      if (fseek(file, bswap_32(pkg_table_entry.offset), SEEK_SET))
        return -1;
      uint32_t filesize = bswap_32(pkg_table_entry.size);
      if (filesize > sizeof(param_sfo))
        filesize = sizeof(param_sfo);
      if (fread(param_sfo, filesize, 1, file) != 1)
        return -1;

      return 0;
    }
  }

  return 1;
}

// Opens a PS4 PKG or param.sfo file and reads its param.sfo content into
// "params"; returns NULL on error, in which case "count" means:
//   0: Error while opening file.
//   1: File is not a PKG.
//   2: Param.sfo magic number not found.
//   3: PKG does not contain param.sfo.
// This function also sets merged_ver (pkgrename.c) if a patch is found
struct sfo_parameter *sfo_read(int *count, const char *filename) {
  // Open binary file
  if ((file = fopen(filename, "rb")) == NULL) {
    *count = 0;
    return NULL;
  }

  // Load param.sfo
  uint32_t magic;
  magic = get_uint32(0);
  if (magic == MAGIC_NUMBER_PKG) {
    switch(load_param_sfo()) {
      case -1:
        *count = 0;
        return NULL;
      case 1:
        *count = 3;
        return NULL;
    }
  } else {
    *count = 1;
    return NULL;
  }

  // Load param.sfo header
  struct sfo_header {
    uint32_t magic;
    uint32_t version;
    uint32_t keytable_offset;
    uint32_t datatable_offset;
    uint32_t indextable_entries;
  } *sfo_header;
  sfo_header = (struct sfo_header *) param_sfo;

  // Check for valid param.sfo magic number
  if (sfo_header->magic != MAGIC_NUMBER_PARAM_SFO) {
    *count = 2;
    return NULL;
  }

  // Load index table
  struct indextable_entry {
    uint16_t keytable_offset;
    uint16_t param_fmt; // Type of data
    uint32_t parameter_length;
    uint32_t parameter_max_length;
    uint32_t datatable_offset;
  } *indextable_entry;
  indextable_entry = (struct indextable_entry *)
    &param_sfo[sizeof(struct sfo_header)];

  // Allocate enough memory for the parameter array
  struct sfo_parameter *params = malloc(sfo_header->indextable_entries
    * sizeof(struct sfo_parameter));

  // Fill the parameter array
  for (size_t i = 0; i < sfo_header->indextable_entries; i++) {
    // Get current parameter's name
    params[i].name = &param_sfo[sfo_header->keytable_offset + indextable_entry[i].keytable_offset];

    // Get current parameter's value
    switch (indextable_entry[i].param_fmt) {
      case 4: // UTF-8 special mode string
        params[i].type = TYPE_SPECIAL_MODE_STRING;
      case 516: // UTF-8 string
        params[i].type = TYPE_STRING;
        params[i].string = &param_sfo[sfo_header->datatable_offset +
          indextable_entry[i].datatable_offset];
        break;
      case 1028: // Integer
        params[i].type = TYPE_INTEGER;
        params[i].integer = (uint32_t *) &param_sfo[sfo_header->
          datatable_offset + indextable_entry[i].datatable_offset];
        break;
      default:
        params[i].type = -1;
    }
  }

  fclose(file);

  *count = sfo_header->indextable_entries;
  return params;
}

// Loads a PKG file's changelog (if it exists) into a buffer
// Returns -1 on read error, otherwise 0
int load_changelog(char *buffer, int bufsize, const char *filename) {
  if ((file = fopen(filename, "rb")) == NULL)
    return -1;

  const int pkg_table_offset = bswap_32(get_uint32(0x018));
  const int pkg_entry_count = bswap_32(get_uint32(0x010));
  struct pkg_table_entry pkg_table_entry;

  // Search PKG file until changeinfo.xml is found
  if (fseek(file, pkg_table_offset, SEEK_SET)) {
    fclose(file);
    return -1;
  }
  for (int i = 0; i < pkg_entry_count; i++) {
    if (fread(&pkg_table_entry, sizeof(struct pkg_table_entry), 1, file) != 1) {
      fclose(file);
      return -1;
    }

    if (pkg_table_entry.id == bswap_32(0x00001260)) { // changeinfo.xml file ID
      if (fseek(file, bswap_32(pkg_table_entry.offset), SEEK_SET)) {
        fclose(file);
        return -1;
      }
      uint32_t filesize = bswap_32(pkg_table_entry.size);
      if (filesize > bufsize - 1)
        filesize = bufsize - 1;
      if (fread(buffer, filesize, 1, file) != 1) {
        fclose(file);
        return -1;
      }

      fclose(file);
      buffer[filesize] = '\0';
      return 0;
    }
  }

  fclose(file);
  buffer[0] = '\0';
  return 0;
}

// Loads the true patch version from a string and stores it in a buffer;
// the buffer must be of size 6
// Returns 1 if the patch version has been found, otherwise 0
int get_patch_version(char *version_buf, const char *changelog) {
  // Grab the highest patch version
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

// Prints a PKG file's param.sfo data.
void print_sfo(const char *filename) {
  int count;
  struct sfo_parameter *params = sfo_read(&count, filename);

  // Exit on error
  if (params == NULL) {
    fprintf(stderr, "Error while reading SFO parameters.\n");
    return;
  }

  for (int i = 0; i < count; i++) {
    switch (params[i].type) {
      case TYPE_SPECIAL_MODE_STRING:
      case TYPE_STRING:
        printf("%s=\"%s\"\n", params[i].name, params[i].string);
        break;
      case TYPE_INTEGER:
        printf("%s=0x%08X\n", params[i].name, *params[i].integer);
        break;
      default:
        printf("Unknown value\n");
    }
  }

  free(params);
}
