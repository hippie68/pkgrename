#include "../include/colors.h"
#include "../include/common.h"
#include "../include/onlinesearch.h"
#include "../include/options.h"

#ifndef _WIN32
#include <curl/curl.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define URL_LEN 128

static int create_url(char url[URL_LEN], char *content_id) {
  char *prefix;
  switch (content_id[0]) {
    case 'U':
      prefix = "https://store.playstation.com/en-us/product/";
      break;
    case 'E':
      prefix = "https://store.playstation.com/en-gb/product/";
      break;
    case 'H':
      prefix = "https://store.playstation.com/en-hk/product/";
      break;
    case 'J':
      prefix = "https://store.playstation.com/ja-jp/product/";
      break;
    default:
      printf("Online search not supported for this Content ID (\"%s\").\n",
        content_id);
      return 1;
  }
  strcpy(url, prefix);
  strcat(url, content_id);
  return 0;
}

#ifdef _WIN32
void search_online(char *content_id, char *title) {
  char url[URL_LEN];
  char cmd[128];

  if (create_url(url, content_id) != 0) return;

  printf("Searching online, please wait...\n");

  // Create cURL command
  strcpy(cmd, "curl.exe -Ls --connect-timeout 5 ");
  strcat(cmd, url);
  //printf("cmd: %s\n", cmd); // DEBUG

  // Run cURL
  FILE *pipe = _popen(cmd, "rb");
  if (pipe == NULL) {
    fprintf(stderr, "Error while calling curl.exe.\n");
    return;
  }
  char c;
  char curl_output[65536];
  int index = 0;
  while((c = fgetc(pipe)) != EOF && index <= 65536) {
    curl_output[index++] = c;
  }
  curl_output[index] = '\0';
  int result = _pclose(pipe);
  if (result != 0) {
    fprintf(stderr, "An error occured (error code \"%d\").\n"
      "See \"https://curl.se/libcurl/c/libcurl-errors.html\".\n", result);
  }

  //printf("Output: %s\n", curl_output); // DEBUG

  // Search cURL's output for the title
  char *start = strstr(curl_output, "@type");
  if (start != NULL && strlen(start) > 25) {
    start += 25;
    char *end = strchr(start, '"');
    if (end != NULL) {
      end[0] = '\0';
      if (strcmp(start, "") != 0) {
        printf("Online title: \"%s\"\n", start);
        strncpy(title, start, MAX_FILENAME_LEN);
        title[MAX_FILENAME_LEN - 1] = '\0';
      } else goto fail; // String is empty
    } else {
      fprintf(stderr, BRIGHT_RED "Error while searching online. Please contact"
        " the developer at \"https://github.com/hippie68/pkgrename/issues\" and "
        "show him this link: \"%s\".\n" RESET, url);
    }
  } else {
    fail:
    printf("No online information found.\n");
  }
}

#else
static char *curl_output = NULL;

// libcurl callback function
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata){
  if (curl_output == NULL) {
    curl_output = malloc(1);
    curl_output[0] = '\0';
  }
  curl_output = realloc(curl_output, strlen(curl_output) + nmemb + 1);
  strcat(curl_output, ptr);
  return(nmemb);
}

void search_online(char *content_id, char *title) {
  char url[URL_LEN];

  CURL *curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "Error while initializing cURL.\n");
    return;
  }

  if (create_url(url, content_id) != 0) goto exit;

  printf("Searching online, please wait...\n");

  CURLcode result;
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "An error occured (error code \"%d\").\n"
      "See https://curl.se/libcurl/c/libcurl-errors.html\n", result);
    goto exit;
  }
  //printf("%s\n", curl_output); // DEBUG
  char *start = strstr(curl_output, "@type");
  if (start != NULL && strlen(start) > 25) {
    start += 25;
    char *end = strchr(start, '"');
    if (end != NULL) {
      end[0] = '\0';
      if (strcmp(start, "") != 0) {
        printf("Online title: \"%s\"\n", start);
        strncpy(title, start, MAX_FILENAME_LEN);
        title[MAX_FILENAME_LEN - 1] = '\0';
      } else goto fail; // String is empty
    } else {
      fprintf(stderr, BRIGHT_RED "Error while searching online. Please contact"
        " the developer at https://github.com/hippie68/pkgrename/issues and "
        "show him this link: %s.\n" RESET, url);
    }
  } else {
    fail:
    printf("No online information found.\n");
  }

  exit:
  curl_easy_cleanup(curl);
  if (curl_output != NULL) {
    free(curl_output);
    curl_output = NULL;
  }
}
#endif
