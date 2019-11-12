#include "T.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define NUM_SAMPLES 50000

FILE *fd;
long int collected_samples[2][NUM_SAMPLES];

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

int main()
{
  int i;
  int target;
  long int sleep_time; //nanoseconds
  long int *target_address;
  char *quixote;
  char file_name[40];
  double cpu_time_used;
  struct timespec request,remain;
  clock_t start, end;  

  printf("Enter target address (int)\n");
  scanf ("%d",&target);
  //quixote = (char *)get_address_quixote(target);
  target_address = (long int *)get_address_table(target);
  printf("Target address %lx \n",target_address);
  //printf("%c \n", *quixote);
  while (1)
  {
    printf("Enter sleep time \n");
    scanf ("%ld",&sleep_time);
    /*Setup timer*/
    request.tv_sec = (int)((sleep_time-10)/1000000000);
    request.tv_nsec = (long)((sleep_time-10)%1000000000);
    snprintf(file_name,40,"sender_%d_%ld.txt",target, sleep_time);
    printf("Saving results to %s %lu %lu\n",file_name,request.tv_sec,request.tv_nsec);
    fd=fopen(file_name,"w");
    if (fd == NULL) fprintf(stderr,"Unable to open file\n");
    for (i = 0; i < NUM_SAMPLES ; i++)
    {
      nanosleep(&request, &remain);
      //collected_samples[0][i] = mem_access(target_address);
      collected_samples[0][i] = access_timed(target_address);
      access_timed(target_address+1);
      access_timed(target_address+2);
      collected_samples[1][i] = timestamp();
    }
    for (i = 0; i < NUM_SAMPLES ; i++)
    {
      fprintf(fd,"%lu %lu\n",collected_samples[0][i],collected_samples[1][i]);
    }
    fclose(fd);
  }
  return 0;
}
