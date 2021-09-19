#include "../include/colors.h"
#include "../include/common.h"
#include "../include/onlinesearch.h"
#include "../include/options.h"

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

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

void search_online(char *content_id, char *title, int manual_search) {
  CURL *curl = curl_easy_init();
  if (curl == NULL) {
    fprintf(stderr, "Error while initializing cURL.\n");
    return;
  }

  if (manual_search) printf("Searching online, please wait...\n");

  char *url_part1;
  switch (content_id[0]) {
    case 'U':
      url_part1 = "https://store.playstation.com/en-us/product/";
      break;
    case 'E':
      url_part1 = "https://store.playstation.com/en-gb/product/";
      break;
    case 'J':
      url_part1 = "https://store.playstation.com/ja-jp/product/";
      break;
    default:
      printf("Online search not supported for this Content ID (\"%s\").\n",
        content_id);
      goto exit;
  }
  char *url_part2 = content_id;
  char *url = malloc(strlen(url_part1) + strlen(url_part2) + 1);
  strcpy(url, url_part1);
  strcat(url, url_part2);
  CURLcode result;
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  result = curl_easy_perform(curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "An error occured (error code \"%d\"). "
      "See https://curl.se/libcurl/c/libcurl-errors.html.\n", result);
    goto exit;
  }
  //printf("%s\n", curl_output); // DEBUG
  char *start = strstr(curl_output, "@type");
  if (start != NULL && strlen(start) > 25) {
    start += 25;
    char *end = strchr(start, '"');
    if (end != NULL) {
      end[0] = '\0';
      if (manual_search) printf("Online title: \"%s\"\n", start);
      strncpy(title, start, MAX_FILENAME_LEN);
      title[MAX_FILENAME_LEN - 1] = '\0';
    } else {
      printf(BRIGHT_RED "Error while searching online. Please contact the"
        "developer and give him this link: %s.\n" RESET, url);
    }
  } else {
    if (option_online == 0) printf("No online information found.\n");
  }

  exit:
  curl_easy_cleanup(curl);
  if (curl_output != NULL) {
    free(curl_output);
    curl_output = NULL;
  }
}
