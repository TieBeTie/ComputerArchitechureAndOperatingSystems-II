/*
    Программа принимает три аргумента: строку с IPv4-адресом,
    и два неотрицательных целых числа, первое из которых определяет
    общее время работы программы timeout,
    а второе - время между отдельными запросами в микросекундах interval.

    Необходимо реализовать упрощённый аналог утилиты ping,
    которая определяет доступность удаленного хоста, используя протокол ICMP.

    Программа должна последовательно отправлять echo-запросы к указанному адресу и
    подсчитывать количество успешных ответов.
    Между запросами, во избежание большой нагрузки на сеть,
    необходимо выдерживать паузу в interval микросекунд
    (для этого можно использовать функцию usleep).

    Через timeout секунд необходимо завершить работу,
    и вывести на стандартный поток вывода количество полученных ICMP-ответов,
    соответствующих запросам.

    В качестве аналога можно посмотреть утилиту /usr/bin/ping.

    Указания: используйте инструменты ping и wireshark для того,
    чтобы исследовать формат запросов и ответов.
    Для того, чтобы выполняемый файл мог без прав администратора
    взаимодействовать с сетевым интерфейсом,
    нужно после компиляции установить ему capabilities командой:
    setcap cat_net_raw+eip PROGRAM.
    Контрольная сумма для ICMP-заголовков вычисляется по алгоритму из RFC-1071.
*/

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
  ERROR = -1,
  BUFFER_SIZE = 8192,
  HEAD_SIZE = 12,
  TALE_SIZE = 5,
  DNS_PORT = 53
} constants;

/*
void Send() {

}

int main(int argc, char *argv[]) {
  char *IPv4 = argv[1];
  uint64_t timeout = strtoll(argv[2], NULL, 10);
  uint64_t interval = strtoll(argv[3], NULL, 10);

  int icmp_fd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
  if(socket_fd == -1) {
    perror("Socket");
    abort();
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr = inet_addr(IPv4),
      .sin_port = htons(53)
  };

  Send();
  recvfrom(icmp_fd, )
  usleep(interval);


  char shared_memory[NAME_MAX] = "/TieBeTie(ami-ypech-25)";

  printf("%s\n", shared_memory);
  fflush(stdout);

  int shm_id = shm_open(shared_memory, O_RDWR | O_CREAT, 0642);
  ftruncate(shm_id, sizeof(shared_data_t));
  shared_data_t *semaphores = mmap(NULL,
                                   sizeof(shared_data_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED,
                                   shm_id,
                                   0);
  close(shm_id);

  sem_init(&semaphores->response_ready, 0642, 0);
  sem_init(&semaphores->request_ready, 0642, 0);

  void *exec_func;
  exec_func = dlopen(argv[1], RTLD_LAZY);
  // проверяем открылась ли разделяемая бибилотека
  if (!exec_func) {
    sem_close(&semaphores->response_ready);
    sem_close(&semaphores->request_ready);
    munmap(semaphores, sizeof(shared_data_t));
    shm_unlink(shared_memory);
    dlclose(exec_func);
    return 0;
  }

  while (1) {
    sem_wait(&semaphores->request_ready);
    if (strlen(semaphores->func_name) == 0) {
      break;
    }
    double (*ret_func)(double) = dlsym(exec_func, semaphores->func_name);
    semaphores->result = ret_func(semaphores->value);
    fflush(stdout);
    sem_post(&semaphores->response_ready);
  }

  sem_close(&semaphores->response_ready);
  sem_close(&semaphores->request_ready);
  munmap(semaphores, sizeof(shared_data_t));
  shm_unlink(shared_memory);
  dlclose(exec_func);
  return 0;
}
*/
// Define the Packet Constants
// ping packet size
#define PING_PKT_S 64

// Automatic port number
#define PORT_NO 0

// Automatic port number
#define PING_SLEEP_RATE 1000000

// Gives the timeout delay for receiving packets
// in seconds
#define RECV_TIMEOUT 1

// Define the Ping Loop
int pingloop = 1;

// ping packet structure
struct ping_pkt {
  struct icmphdr hdr;
  char msg[PING_PKT_S - sizeof(struct icmphdr)];
};

// Calculating the Check Sum
unsigned short checksum(void *b, int len) {
  unsigned short *buf = b;
  unsigned int sum = 0;
  unsigned short result;

  for (sum = 0; len > 1; len -= 2)
    sum += *buf++;
  if (len == 1)
    sum += *(unsigned char *) buf;
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  result = ~sum;
  return result;
}

// Interrupt handler
void intHandler(int dummy) {
  pingloop = 0;
}

// Performs a DNS lookup
char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con) {
  printf("\nResolving DNS..\n");
  struct hostent *host_entity;
  char *ip = (char *) malloc(NI_MAXHOST * sizeof(char));
  int i;

  if ((host_entity = gethostbyname(addr_host)) == NULL) {
    // No ip found for hostname
    return NULL;
  }

  //filling up address structure
  strcpy(ip, inet_ntoa(*(struct in_addr *)
      host_entity->h_addr));

  (*addr_con).sin_family = host_entity->h_addrtype;
  (*addr_con).sin_port = htons(PORT_NO);
  (*addr_con).sin_addr.s_addr = *(long *) host_entity->h_addr;

  return ip;

}

// Resolves the reverse lookup of the hostname
char *reverse_dns_lookup(char *ip_addr) {
  struct sockaddr_in temp_addr;
  socklen_t len;
  char buf[NI_MAXHOST], *ret_buf;

  temp_addr.sin_family = AF_INET;
  temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
  len = sizeof(struct sockaddr_in);

  if (getnameinfo((struct sockaddr *) &temp_addr, len, buf,
                  sizeof(buf), NULL, 0, NI_NAMEREQD)) {
    printf("Could not resolve reverse lookup of hostname\n");
    return NULL;
  }
  ret_buf = (char *) malloc((strlen(buf) + 1) * sizeof(char));
  strcpy(ret_buf, buf);
  return ret_buf;
}

// make a ping request
void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr,
               char *ping_dom, char *ping_ip, char *rev_host, uint64_t interval, uint64_t timeout) {
  int ttl_val = 64, msg_count = 0, i, addr_len, flag = 1,
      msg_received_count = 0;

  struct ping_pkt pckt;
  struct sockaddr_in r_addr;
  struct timespec time_start, time_end, tfs, tfe;
  long double rtt_msec = 0, total_sec = 0;
  struct timeval tv_out;
  double timeElapsed = 0;
  tv_out.tv_sec = RECV_TIMEOUT;
  tv_out.tv_usec = 0;

  clock_gettime(CLOCK_MONOTONIC, &tfs);


  // set socket options at ip to TTL and value to 64,
  // change to what you want by setting ttl_val
  if (setsockopt(ping_sockfd, SOL_IP, IP_TTL,
                 &ttl_val, sizeof(ttl_val)) != 0) {
    printf("\nSetting socket option to TTL failed!\n");
    return;
  } else {
    printf("\nSocket set to TTL..\n");
  }

  // setting timeout of recv setting
  setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO,
             (const char *) &tv_out, sizeof tv_out);

  // send icmp packet in an infinite loop
  while (total_sec < timeout) {
    // flag is whether packet was sent or not
    printf("%f %d\n",(double) total_sec, timeout);
    fflush(stdout);
    flag = 1;

    //filling packet
    bzero(&pckt, sizeof(pckt));

    pckt.hdr.type = ICMP_ECHO;
    pckt.hdr.un.echo.id = getpid();

    for (i = 0; i < sizeof(pckt.msg) - 1; i++)
      pckt.msg[i] = i + '0';

    pckt.msg[i] = 0;
    pckt.hdr.un.echo.sequence = msg_count++;
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

    usleep(interval);

    //send packet
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0,
               (struct sockaddr *) ping_addr,
               sizeof(*ping_addr)) <= 0) {
      printf("\nPacket Sending Failed!\n");
    }

    //receive packet
    addr_len = sizeof(r_addr);

    if (recvfrom(ping_sockfd, &pckt, sizeof(pckt), 0,
                 (struct sockaddr *) &r_addr, &addr_len) <= 0
        && msg_count > 1) {
      printf("\nPacket receive failed!\n");
    } else {
      clock_gettime(CLOCK_MONOTONIC, &time_end);

      timeElapsed = ((double) (time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
      rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0
          + timeElapsed;


      clock_gettime(CLOCK_MONOTONIC, &tfe);
      timeElapsed = ((double) (tfe.tv_sec - tfs.tv_sec)) / 1000000.0;

      total_sec += (tfe.tv_sec - tfs.tv_sec) + rtt_msec / 1000;

      printf("%d bytes from %s (h: %s)( %s)msg_seq =%d ttl =%d rtt = %Lf ms.\n",
             PING_PKT_S, ping_dom, rev_host,
             ping_ip, msg_count,
             ttl_val, rtt_msec);

      msg_received_count++;
    }
  }

}

// Driver Code
int main(int argc, char *argv[]) {
  char *IPv4 = argv[1];
  uint64_t timeout = strtoll(argv[2], NULL, 10);
  uint64_t interval = strtoll(argv[3], NULL, 10);

  int sockfd;
  char *ip_addr, *reverse_hostname;
  struct sockaddr_in addr_con;
  int addrlen = sizeof(addr_con);
  char net_buf[NI_MAXHOST];

  ip_addr = dns_lookup(argv[1], &addr_con);
  if (ip_addr == NULL) {
    printf("\nDNS lookup failed! Could not resolve hostname!\n");
    return 0;
  }

  reverse_hostname = reverse_dns_lookup(ip_addr);
  printf("\nTrying to connect to '%s' IP: %s\n", argv[1], ip_addr);
  printf("\nReverse Lookup domain: %s",
         reverse_hostname);

  //socket()
  sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sockfd < 0) {
    printf("\nSocket file descriptor not received!!\n");
    return 0;
  } else
    printf("\nSocket file descriptor %d received\n", sockfd);

  signal(SIGINT, intHandler);//catching interrupt

  //send pings continuously
  send_ping(sockfd, &addr_con, reverse_hostname,
            ip_addr, argv[1], interval, timeout);

  return 0;
}

