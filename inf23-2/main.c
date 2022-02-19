//  В аргументе командной строки передается полный URL веб-страницы в формате HTML.
//
//  Необходимо загрузить эту страницу и вывести на стандартный поток вывода
//  только заголовок страницы, заключенный между тегами <title> и </title>.
//
//  Используйте LibCURL. На сервер нужно отпарвить только исходный файл,
//  который будет скомпилирован и слинкован с нужными опциями.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <string.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <curl/curl.h>

typedef enum {
  PERRROR = 1,
  ERROR = -1,
  BUFFER_SIZE = 8192,
  MEGA_BUFFER_SIZE = 1000000,
  HTTP_PORT = 80
} constants;

typedef struct {
  char *data;
  int64_t length;
  int64_t capacity;
} buffer_t;

static int64_t CallbackFunction(
    char *ptr,
    int64_t chunk_size,
    int64_t nmemb,
    void *user_data
) {
  buffer_t *buffer = user_data;
  int64_t total_size = chunk_size * nmemb;
  int64_t required_size = buffer->length + total_size;

  if (required_size > buffer->capacity) {
    required_size *= 2;
    buffer->data = realloc(buffer->data, required_size);
    buffer->capacity = required_size;
  }

  memcpy(buffer->data + buffer->length, ptr, total_size);
  buffer->length += total_size;
  return total_size;
}

void PrintTitle(buffer_t *buffer) {
  char *data = buffer->data;
  char *head = strstr(data, "<title>");
  char *tale = strstr(data, "</title>");

  for (int i = 0; i < tale - head - 7; i++) {
    printf("%c", *(head + i + 7));
  }
}

int main(int argc, char *argv[]) {
  const char *url = argv[1];

  CURL *curl = curl_easy_init();
  CURLcode result;

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallbackFunction);

  buffer_t buffer = {.data = NULL,
                     .length = 0,
                     .capacity = 0};

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  result = curl_easy_perform(curl);

  PrintTitle(&buffer);

  free(buffer.data);
  curl_easy_cleanup(curl);
}