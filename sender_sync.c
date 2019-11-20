#include "T.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define THRESHOLD 20000 / 2
#define PACKET_SIZE 24
#define FILE_NAME_SIZE 80

#define lfence() __asm__ volatile("lfence;");

FILE *fd;

struct sockaddr_in server;
struct sockaddr_in client;
socklen_t clientlen;
int s, r;
int target;
long int *target_address;
char *quixote;
char file_name[FILE_NAME_SIZE];
char in[2 * PACKET_SIZE];
char out[PACKET_SIZE];

/*Get the value of rdtsc*/
unsigned long int timestamp(void)
{
  unsigned long int result;
  unsigned int bottom;
  unsigned int top;
  asm volatile("rdtsc"
               : "=a"(bottom), "=d"(top));
  result = top;
  result = (result << 32) & 0xFFFFFFFF00000000UL;
  return (result | bottom);
}

/*Get the value of a memory location*/
long int mem_access(long int *v)
{
  long int rv = 0;
  asm volatile(
      "movq (%1), %0"
      : "+r"(rv)
      : "r"(v)
      :);
  return rv;
}

/*Measure read time*/
int access_timed(long int *pos_data)
{
  volatile unsigned int time;
  asm __volatile__(
      //" mfence \n"
      //" lfence \n"
      " rdtsc \n"
      " lfence \n"
      " movl %%eax, %%esi \n"
      " movl (%1), %%eax \n"
      " lfence \n"
      " rdtsc \n"
      " subl %%esi, %%eax \n"
      : "=a"(time)
      : "c"(pos_data)
      : "%esi", "%edx");
  return time;
}

void handle(char out[PACKET_SIZE], char in[PACKET_SIZE * 2])
{
  int i;
  int res = 0;
  for (i = 0; i < PACKET_SIZE - sizeof(uint64_t); ++i)
    out[i] = in[i]; //Authenticate on the attacker side
  uint64_t value = (uint64_t) * ((unsigned long int *)(in + PACKET_SIZE - sizeof(uint64_t)));
  lfence();
  if (value > THRESHOLD)
  {
    res = access_timed(target_address);
  }
  lfence();
  fprintf(fd, "%i %lu\n", res, timestamp());
  fflush(fd);
}

int main(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Expected IP address and target address (int)\n");
    return -1;
  }

  if (!inet_aton(argv[1], &server.sin_addr))
  {
    printf("Wrong IP \n");
    return -1;
  }
  server.sin_family = AF_INET;
  server.sin_port = htons(10000);
  s = socket(AF_INET, SOCK_DGRAM, 0);

  if (s == -1)
    return 2;

  if (bind(s, (struct sockaddr *)&server, sizeof server) == -1)
    return 3;

  target = atoi(argv[2]);
  target_address = (long int *)get_address_table(target);
  printf("Target address %i %lx \n", target, (long int)target_address);
  snprintf(file_name, FILE_NAME_SIZE, "sender_sync_%i.txt", target);
  printf("Saving results to %s\n", file_name);

  fd = fopen(file_name, "w");
  if (fd == NULL)
    fprintf(stderr, "Unable to open file\n");

  while (1)
  {
    clientlen = sizeof client;
    r = recvfrom(s, in, sizeof in, 0, (struct sockaddr *)&client, &clientlen);
    if (r < PACKET_SIZE)
      continue;
    handle(out, in);
    sendto(s, out, PACKET_SIZE, 0, (struct sockaddr *)&client, clientlen);
  }

  fclose(fd);
  //printf("%c \n", *quixote);
  return 0;
}
