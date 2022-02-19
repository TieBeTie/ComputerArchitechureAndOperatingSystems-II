//  Программе на стандартный поток ввода передается последовательность байт.
//
//  Необходимо вычислить контрольную сумму SHA-512 и вывести это значение в hex-виде c префиксом 0x.
//
//  Используйте API OpenSSL/LibreSSL. Запуск сторонних команд через fork+exec запрещен.
//
//  Отправляйте только исходный файл Си-программы с решением.

#include <openssl/sha.h>
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

typedef enum {
  BUFFER_SIZE = 64
} constants;

int main() {
  SHA512_CTX sha512_ctx;
  SHA512_Init(&sha512_ctx);
  unsigned char buffer[BUFFER_SIZE];
  uint64_t n;

  while ((n = read(0, buffer, sizeof(buffer))) > 0) {
    SHA512_Update(&sha512_ctx, buffer, n);
  }

  unsigned char hash[BUFFER_SIZE];
  SHA512_Final(hash, &sha512_ctx);

  printf("0x");
  for (int i = 0; i < sizeof(hash); ++i) {
    printf("%02x", hash[i] & 0xff);
  }
}