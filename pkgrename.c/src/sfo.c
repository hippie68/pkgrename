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

// Global variables
static FILE *file;
static size_t pkg_offset;

#ifdef _WIN32
// Replacement function for byteswap.h's bswap_16
static uint16_t bswap_16(uint16_t val) {
  return (val >> 8) | (val << 8);
}

// Replacement function for byteswap.h's bswap_32
static uint32_t bswap_32(uint32_t val) {
  val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0x00FF00FF );
  return (val << 16) | (val >> 16);
}
#endif

// Returns "len" bytes, starting at specified offset
static int get_bytes(size_t offset, int len) {
  int bytes;
  fseek(file, pkg_offset + offset, SEEK_SET);
  fread(&bytes, len, 1, file);
  return bytes;
}

// Finds the param.sfo's offset inside a PS4 PKG file
static void get_pkg_offset() {
  const int pkg_table_offset = bswap_32(get_bytes(0x018, 4));
  const int pkg_file_count = bswap_32(get_bytes(0x00C, 4));
  struct pkg_table_entry {
    uint32_t id;
    uint32_t filename_offset;
    uint32_t flags1;
    uint32_t flags2;
    uint32_t offset;
    uint32_t size;
    uint64_t padding;
  } pkg_table_entry[pkg_file_count];
  fseek(file, pkg_table_offset, SEEK_SET);
  fread(pkg_table_entry, sizeof(struct pkg_table_entry) * pkg_file_count, 1, file);
  for (int i = 0; i < pkg_file_count; i++) {
    if (pkg_table_entry[i].id == 1048576) { // param.sfo file ID
      pkg_offset = bswap_32(pkg_table_entry[i].offset);
      return;
    }
  }
}

// Opens a PS4 PKG or param.sfo file and reads its param.sfo content into
// "params"; returns NULL on error, in which case "count" means:
//   0: Error while opening file
//   1: Param.sfo magic number not found in file
struct sfo_parameter *sfo_read(int *count, char *filename) {
  // Open binary file
  if ((file = fopen(filename, "rb")) == NULL) {
    *count = 0;
    return NULL;
  }

  // Read magic number
  pkg_offset = 0;
  int magic = get_bytes(0, 4);

  // Set param.sfo offset and size,
  // if file is a PKG
  if (magic == MAGIC_NUMBER_PKG) {
    get_pkg_offset();
    magic = get_bytes(0, 4); // get_bytes() now knows the PKG's offset
  } else {
    pkg_offset = 0;
  }

  // Check for valid param.sfo magic number
  if (magic != MAGIC_NUMBER_PARAM_SFO) {
    *count = 1;
    return NULL;
  }

  // Load param.sfo header
  struct {
    uint32_t magic;
    uint32_t version;
    uint32_t keytable_offset;
    uint32_t datatable_offset;
    uint32_t indextable_entries;
  } sfo_header;
  fseek(file, pkg_offset, SEEK_SET);
  fread(&sfo_header, sizeof(sfo_header), 1, file);

  // Define the index table
  const int indextable_offset = 0x14;
  const int indextable_entry_size = 0x10;
  struct indextable_entry {
    uint16_t keytable_offset;
    uint16_t param_fmt; // Type of data
    uint32_t parameter_length;
    uint32_t parameter_max_length;
    uint32_t datatable_offset;
  } indextable_entry[sfo_header.indextable_entries];

  // Allocate enough memory for the parameter array
  struct sfo_parameter *params = malloc(sfo_header.indextable_entries
    * sizeof(struct sfo_parameter));

  // Fill the parameter array
  for (int i = 0; i < sfo_header.indextable_entries; i++) {
    fseek(file, pkg_offset + indextable_offset + i * indextable_entry_size, SEEK_SET);
    fread(&indextable_entry[i], sizeof(struct indextable_entry), 1, file);

    // Get current parameter's name
    fseek(file, pkg_offset + sfo_header.keytable_offset + indextable_entry[i].keytable_offset, SEEK_SET);
    fread(params[i].name, PARAM_NAME_SIZE, 1, file);
    params[i].name[PARAM_NAME_SIZE] = '\0';

    // Get current parameter's value
    switch (indextable_entry[i].param_fmt) {
      case 4: // UTF-8 special mode string
        params[i].type = TYPE_SPECIAL_MODE_STRING;
      case 516: // UTF-8 string
        params[i].type = TYPE_STRING;
        fseek(file, pkg_offset + sfo_header.datatable_offset + indextable_entry[i].datatable_offset, SEEK_SET);
        fread(params[i].string, indextable_entry[i].parameter_length, 1, file);
        params[i].string[indextable_entry[i].parameter_length - 1] = '\0';
        break;
      case 1028: // Integer
        params[i].type = TYPE_INTEGER;
        fseek(file, pkg_offset + sfo_header.datatable_offset + indextable_entry[i].datatable_offset, SEEK_SET);
        fread(&params[i].integer, 4, 1, file);
        break;
      default:
        params[i].type = -1;
    }
  }

  fclose(file);

  *count = sfo_header.indextable_entries;
  return params;
}

// Prints a PKG or param.sfo file's SFO data.
void print_sfo(char *filename) {
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
        printf("%s=0x%08X\n", params[i].name, params[i].integer);
        break;
      default:
        printf("Unknown value\n");
    }
  }

  free(params);
}
