#include "cache_utils.h"

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

/* parity and addr2lice_linear functions are the ones defined in the Mastik tool */
/* Mastik is available in https://cs.adelaide.edu.au/~yval/Mastik/ */
int parity(uint64_t v)
{
    v ^= v >> 1;
    v ^= v >> 2;
    v = (v & 0x1111111111111111UL) * 0x1111111111111111UL;
    return (v >> 60) & 1;
}

int addr2slice_linear(uintptr_t addr, int slices)
{
    int bit0 = parity(addr & SLICE_MASK_0);
    int bit1 = parity(addr & SLICE_MASK_1);
    int bit2 = parity(addr & SLICE_MASK_2);
    return ((bit2 << 2) | (bit1 << 1) | bit0) & (slices - 1);
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

/*Measure read time*/
int access_timed_full(long int *pos_data)
{
    volatile unsigned int time;
    unsigned long int t1 = timestamp();
    lfence();
    mem_access(pos_data);
    lfence();
    time = (int)(timestamp() - t1);
    return time;
}

/*Measure read time and flush data afterwards*/
int access_timed_flush(long int *pos_data)
{
    volatile unsigned int time;
    asm __volatile__(
        " mfence \n"
        " lfence \n"
        " rdtsc \n"
        " lfence \n"
        " movl %%eax, %%esi \n"
        " movl (%1), %%eax \n"
        " lfence \n"
        //" mfence \n"
        " rdtsc \n"
        " subl %%esi, %%eax \n"
        " clflush 0(%1) \n"
        : "=a"(time)
        : "c"(pos_data)
        : "%esi", "%edx");
    return time;
}

/*Measure read time and flush data afterwards*/
int access_timed_full_flush(long int *pos_data)
{
    volatile unsigned int time;
    unsigned long int t1 = timestamp();
    lfence();
    mem_access(pos_data);
    lfence();
    time = (int)(timestamp() - t1);
    flush_data(pos_data);
    return time;
}

/*Flush data from the cache*/
void flush_data(long int *pos_data)
{
    asm __volatile__(
        " clflush 0(%0) \n"
        :
        : "c"(pos_data));
}

int flush_timed(long int *pos_data) // For FLUSH+FLUSH tests
{
    volatile unsigned int time;
    asm __volatile__(
        " mfence \n"
        " lfence \n"
        " rdtsc \n"
        " movl %%eax, %%esi \n"
        " clflush 0(%1) \n"
        " lfence \n"
        " rdtsc \n"
        " subl %%esi, %%eax \n"
        : "=a"(time)
        : "c"(pos_data)
        : "%esi", "%edx");
    return time;
}

/*Probes a linked list of elements*/
int probe_one_set(long int *pos_data)
{
    volatile int time;
#if CACHE_SET_SIZE == 12
    asm __volatile__(
        " rdtsc \n"
        " movl %%eax, %%esi \n"
        " mfence \n"
        " lfence \n"
        " movq 0(%1), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " lfence \n"
        " mfence \n"
        " rdtsc \n"
        " subl %%esi, %%eax \n"
        : "=a"(time)
        : "c"(pos_data)
        : "%esi", "%edx", "%rdi");
#elif CACHE_SET_SIZE == 16
    asm __volatile__(
        //" movq 0(%1), %%rdi \n"
        " rdtsc \n"
        " movl %%eax, %%esi \n"
        " mfence \n"
        " lfence \n"
        " movq 0(%1), %%rdi \n"
        //" movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " lfence \n"
        " mfence \n"
        " rdtsc \n"
        " subl %%esi, %%eax \n"
        : "=a"(time)
        : "c"(pos_data)
        : "%esi", "%edx", "%rdi");
#else
    unsigned long *begin = (unsigned long *)pos_data;
    unsigned long int t1 = timestamp();
    lfence();
    do
    {
        begin = (unsigned long *)(*begin);
    } while (begin != (unsigned long *)pos_data);
    lfence();
    time = (int)(timestamp() - t1);
#endif
    return time;
}

/*Code based on the Rowhammer.js paper, intended to achieve fast evictions*/
int fast_prime(long int ev_set[], int S, int C, int D)
{
    int i, j, k;
    volatile int bas;
    unsigned long int t1 = timestamp();
    lfence();
    for (i = 0; i <= (S - D); i++)
    {
        for (j = 0; j < C; j++)
        {
            for (k = 0; k < D; k++)
            {
                //bas = *((long int *)(ev_set[i + k]));
                mem_access((long int *)(ev_set[i + k]));
            }
        }
    }
    lfence();
    return (int)(timestamp() - t1);
}

/*The reload step does not depend on the size of the cache set (Number of ways)*/
/*Time can be measured at different instants*/
int reload_step(long int *pos_data, long int *conf_data, long int *first_el)
{
    volatile unsigned int time;
    asm __volatile__(
        " mfence \n"
        " lfence \n"
        " rdtsc \n"
        " movl %%eax, %%esi \n"
        " movl 0(%2), %%ebx \n" //Force a miss
        " mfence \n"
        " clflush 0(%2) \n" //Flush that data
        //" mfence \n"
        " lfence \n"
        //" rdtsc \n"
        //" movl %%eax, %%esi \n"
        //" rdtsc \n"
        //" movl %%eax, %%esi \n"
        " movl (%1), %%ebx \n" //Read Target
        " mfence \n"
        //" rdtsc \n"
        //" subl %%esi, %%eax \n"
        //" rdtsc \n"
        " clflush 0(%1) \n" //Flush target
        " mfence \n"
        " movl (%1), %%ebx \n" //Reload target
        " lfence \n"
        " rdtsc \n"
        " subl %%esi, %%eax \n"
        " movl (%3), %%ebx \n" //Read first element of the array
        " mfence \n"
        //" rdtsc \n"
        //" subl %%esi, %%eax \n"
        : "=a"(time)
        : "c"(pos_data), "rr"(conf_data), "ra"(first_el)
        : "%esi", "%edx", "%ebx");
    return time;
}

int refresh_step(long int *pos_data) //Load from second element
{
    volatile unsigned int time;
#if CACHE_SET_SIZE == 12
    asm __volatile__(
        " mfence \n"
        " rdtsc \n"
        " movl %%eax, %%esi \n"
        " lfence \n"
        " movq 0(%1), %%rdi \n"
        " lfence \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        //" movq (%%rdi), %%rdi \n"
        " lfence \n"
        " rdtsc \n"
        " subl %%esi, %%eax \n"
        " mfence \n"
        : "=a"(time)
        : "c"(pos_data)
        : "%esi", "%edx", "%rdi");
#elif CACHE_SET_SIZE == 16
    asm __volatile__(
        " mfence \n"
        " rdtsc \n"
        " movl %%eax, %%esi \n"
        " lfence \n"
        " movq 0(%1), %%rdi \n"
        " mfence \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " movq (%%rdi), %%rdi \n"
        " mfence \n"
        " rdtsc \n"
        " subl %%esi, %%eax \n"
        " mfence \n"
        : "=a"(time)
        : "c"(pos_data)
        : "%esi", "%edx", "%rdi");
#else
    unsigned long *begin = (unsigned long *)(pos_data); //First read
    int cont = 2;
    unsigned long int t1 = timestamp();
    lfence();
    do
    {
        begin = (unsigned long *)(*begin);
        cont++;
    } while (cont < CACHE_SET_SIZE);
    lfence();
    time = (int)(timestamp() - t1);
#endif
    return time;
}

/*Functions for eviction set generation*/

/*Generates an array of candidates starting from the base address of the memory region reserved*/
void generate_candidates_array(long int *base_address, long int candidates_set[], int num_candidates, int tar_set)
{
    int i;
    long int original_value = (long int)base_address;
    original_value += (tar_set << BITS_LINE);
    for (i = 0; i < num_candidates; ++i)
    {
        candidates_set[i] = original_value + (NEXT_SET * i);
    }
}

/*Flush desired set*/
void flush_desired_set(long int address_array[], int len)
{
    int i;
    for (i = 0; i < len; ++i)
    {
        flush_data((long int *)address_array[i]);
    }
}

int check_inside(long int dato, long int array_data[], int array_length)
{
    int i;
    int si = 0;
    for (i = 0; i < array_length; ++i)
    {
        if (dato == array_data[i])
        {
            si = 1;
            break;
        }
    }
    return si;
}

//randomize_set mixes the elements of an array
void randomize_set(long int array[], int len)
{
    int i;
    int random_number;
    for (i = 0; i < len; ++i)
    {
        random_number = rand() % (len);
        long int original = array[i];
        array[i] = array[random_number];
        array[random_number] = original;
    }
}

/*Test if an element is evicted by an array of candidates*/
int probe_candidate(int number_elements, long int array_set[], long int *dir_candidate, int time_main_memory)
{
    int i;
    long u = mem_access(dir_candidate);
    lfence();
    for (i = 0; i < number_elements; ++i)
    {
        mem_access((long int *)array_set[i]);
    }
    lfence();
    return access_timed(dir_candidate) > time_main_memory;
}

/*Generate a filtered_array of candidates from a bigger set*/
/*The filtered array contains all the elements in one set, all the slices*/
int create_filtered_set(long int filtered_set[], long int original_set[], int len, int time_main_memory)
{
    int i;
    int cont_candidates = 0;
#if SLICE_HASH_AVAILABLE
    int number_of_el_slice[CACHE_SLICES] = {0};
    int slice_id;
    uintptr_t phys_addr;
    for (i = 0; i < len; ++i)
    {
        /*Get slice number*/
        int res = virt_to_phys(&phys_addr, getpid(), (uintptr_t)original_set[i]);
        if (res < 0)
        {
            printf("Error \n");
            return -1;
        }
        slice_id = addr2slice_linear(phys_addr, CACHE_SLICES);
        if (number_of_el_slice[slice_id] < CACHE_SET_SIZE)
        {
            filtered_set[CACHE_SET_SIZE * slice_id + number_of_el_slice[slice_id]] = original_set[i];
            number_of_el_slice[slice_id]++;
            cont_candidates++;
        }
        //printf("Cand %i %i %lx %lx\n",cont_candidates,slice_id,phys_addr,original_set[i]);
        if (cont_candidates == CACHE_SET_SIZE * CACHE_SLICES)
            break;
    }
#else
    for (i = 0; i < len; ++i)
    {
        flush_desired_set(original_set, len); //Flushes all the original case (Just int case)
        long int *test_el = (long int *)original_set[i];
        if (!(probe_candidate(cont_candidates, filtered_set, test_el, time_main_memory)))
        {
            filtered_set[cont_candidates] = original_set[i];
            cont_candidates++;
        }
    }
#endif
    return cont_candidates;
}

/*Based on the "Last level cache attacks are practical" paper*/
/*Orders all the elements of the filtered_set per slices writes the eviction set */
/*It is only required if no access to the physical addresses is available */
int create_eviction_set(long int eviction_set[], long int filtered_set[], int len_filtered_set, long int original_set[], int len_original_set, int time_main_memory)
{
    int i;
    int j;
    int num_evicted = 0;
    long int fil_set[len_filtered_set];
    ///copy of the filtered set to be reduced
    for (i = 0; i < len_filtered_set; ++i)
    {
        fil_set[i] = filtered_set[i];
    }
    int len_fil = len_filtered_set;
    for (i = 0; i < len_original_set; ++i)
    {
        if (!(check_inside(original_set[i], fil_set, len_fil)))
        {
            long *test_pointer = (long int *)original_set[i];
            //Check if the address is suitable to order the set, that is, conflicts with the filtered set
            if (probe_candidate(len_fil, fil_set, test_pointer, time_main_memory))
            {
                //It is suitable
                int last_evicted_cont = num_evicted;
                for (j = 0; j < len_fil; ++j)
                {
                    //Create an array with all the elements except the tested one
                    int new_cont = 0;
                    long int array1[len_fil];
                    int k;
                    for (k = 0; k < len_fil; ++k)
                    {
                        if (k != j)
                        {
                            array1[new_cont] = fil_set[k];
                            new_cont++;
                        }
                    }
                    //Set with all the lines except for the tested one
                    if (!(probe_candidate(new_cont, array1, test_pointer, time_main_memory)))
                    {
                        eviction_set[num_evicted] = fil_set[j];
                        num_evicted++;
                    }
                }
                ////Assume the values have been correctly determined if the number of elements is CACHE_SET_SIZE, if not "reset"
                if ((num_evicted - last_evicted_cont) != CACHE_SET_SIZE)
                {
                    num_evicted = last_evicted_cont;
                }
                else
                {
                    ///Update the copy of the filtered_set
                    long int array2[len_fil];
                    int nc = 0;
                    int k;
                    for (k = 0; k < len_fil; ++k)
                    {
                        //Place the element in the new array if not present in the eviction_set
                        if (!check_inside(fil_set[k], eviction_set, num_evicted))
                        {
                            array2[nc] = fil_set[k];
                            nc++;
                        }
                    }
                    //Copy updated values in the fil_set and update its length
                    for (k = 0; k < nc; ++k)
                    {
                        fil_set[k] = array2[k];
                        ;
                    }
                    len_fil = nc;
                    if (nc == 0)
                        break;
                }
            }
        }
    }
    return num_evicted;
}

void initialize_sets(long int eviction_set[], long int filtered_set[], int len_filtered_set, long int original_set[], int len_original_set, int time_main_memory)
{
    int init, i = 0;
    int init2 = 0;
#if SLICE_HASH_AVAILABLE
    while (init2 != CACHE_SET_SIZE * CACHE_SLICES)
    {
        randomize_set(original_set, len_original_set);
        init2 = create_filtered_set(filtered_set, original_set, len_original_set, time_main_memory);
    }
    for (i = 0; i < CACHE_SET_SIZE * CACHE_SLICES; ++i)
    {
        eviction_set[i] = filtered_set[i];
    }
#else
    while (init2 != CACHE_SET_SIZE * CACHE_SLICES)
    {
        randomize_set(original_set, len_original_set);
        init = create_filtered_set(filtered_set, original_set, len_original_set, time_main_memory);
        while (init != CACHE_SET_SIZE * CACHE_SLICES)
        {
            init = create_filtered_set(filtered_set, original_set, len_original_set, time_main_memory);
        }
        init2 = create_eviction_set(eviction_set, filtered_set, init, original_set, len_original_set, time_main_memory);
    }
#endif
}

/*Store the invariant part of the eviction set to later create eviction sets based on it*/
void store_invariant_part(long int eviction_set[CACHE_SET_SIZE * CACHE_SLICES], long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES])
{
    int i;
    for (i = 0; i < CACHE_SET_SIZE * CACHE_SLICES; ++i)
    {
        long int dir_mem = eviction_set[i];
        invariant_part[i] = dir_mem & MASC_SET_LINE_INV;
    }
}

/*Create an eviction set (for all the slices) using part of the virtual addresses*/
void generate_new_eviction_set(int set, long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES], long int new_ev_set[CACHE_SET_SIZE * CACHE_SLICES])
{
    int i;
    for (i = 0; i < CACHE_SET_SIZE * CACHE_SLICES; ++i)
    {
        long int dir_mem = invariant_part[i] + ((set << BITS_LINE) & MASC_SET_LINE);
        new_ev_set[i] = dir_mem;
    }
}

void write_linked_list(long int set_desired[CACHE_SET_SIZE * CACHE_SLICES])
{
    int i;
    //Prime set
    for (i = 0; i < CACHE_SET_SIZE * CACHE_SLICES; ++i)
    {
        long int *dir_mem = (long int *)set_desired[i];
        long int dir_sig;
        if ((i % CACHE_SET_SIZE) != (CACHE_SET_SIZE - 1))
        {
            dir_sig = set_desired[i + 1];
        }
        else
        {
            dir_sig = set_desired[(i / CACHE_SET_SIZE) * CACHE_SET_SIZE];
        }
        *(dir_mem) = dir_sig;
    }
    //Probe set
    for (i = 0; i < CACHE_SET_SIZE * CACHE_SLICES; ++i)
    {
        long int *dir_mem = (long int *)(set_desired[CACHE_SET_SIZE * CACHE_SLICES - 1 - i] + OFF);
        long int dir_sig;
        if ((i % CACHE_SET_SIZE) != (CACHE_SET_SIZE - 1))
        {
            dir_sig = set_desired[CACHE_SET_SIZE * CACHE_SLICES - 2 - i] + OFF;
        }
        else
        {
            dir_sig = set_desired[CACHE_SET_SIZE * CACHE_SLICES - 1 - (i / CACHE_SET_SIZE) * CACHE_SET_SIZE] + OFF;
        }
        *(dir_mem) = dir_sig;
    }
}

void profile_address(long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES], long int new_ev_set[CACHE_SET_SIZE * CACHE_SLICES], long int *target_address, int *set, int *slice)
{
    int i, j, k;
    int cache_model[SETS_PER_SLICE][CACHE_SLICES] = {0};
#if SLICE_HASH_AVAILABLE
    uintptr_t phys_addr;
    int res = virt_to_phys(&phys_addr, getpid(), (uintptr_t)target_address);
    if (res < 0)
    {
        printf("Error \n");
        return;
    }
    *set = (int)((phys_addr >> BITS_LINE) & (MASC_BITS_SET));
    /*Profiling is necessary to get the slice value, sometimes addr2slice_linear(phys_addr, CACHE_SLICES) and the output of the profile are not the same, probably I did something wrong*/
    i = (*set);
    generate_new_eviction_set(i, invariant_part, new_ev_set);
    write_linked_list(new_ev_set);
    for (j = 0; j < CACHE_SLICES; j++)
    {
        long int *prime_address = ((long int *)new_ev_set[j * CACHE_SET_SIZE]); //Get initial address of the eviction set
        for (k = 0; k < 1000; k++)
        {
            int tprime = probe_one_set(prime_address);
            lfence();
            mem_access(target_address);
            lfence();
            int tprime1 = probe_one_set(prime_address);
            cache_model[i][j] += (tprime1 - tprime) / 200;
        }
    }

    int max = 0;
    i = (*set);

    for (j = 0; j < CACHE_SLICES; j++)
    {
        if (cache_model[i][j] > max)
        {
            max = cache_model[i][j];
            *set = i;
            *slice = j;
        }
    }

#else
    int initial_set = (int)((((long int)target_address) >> BITS_LINE) & MASC_KNOWN_SET_BITS);
    for (i = initial_set; i < SETS_PER_SLICE; i += NEXT_SET_NP)
    {
        generate_new_eviction_set(i, invariant_part, new_ev_set);
        write_linked_list(new_ev_set);
        for (j = 0; j < CACHE_SLICES; j++)
        {
            long int *prime_address = ((long int *)new_ev_set[j * CACHE_SET_SIZE]); //Get initial address of the eviction set
            for (k = 0; k < 1000; k++)
            {
                int tprime = probe_one_set(prime_address);
                lfence();
                mem_access(target_address);
                lfence();
                int tprime1 = probe_one_set(prime_address);
                cache_model[i][j] += (tprime1 - tprime) / 200;
            }
        }
    }
    int max = 0;
    for (i = initial_set; i < SETS_PER_SLICE; i += NEXT_SET_NP)
    {
        for (j = 0; j < CACHE_SLICES; j++)
        {
            if (cache_model[i][j] > max)
            {
                max = cache_model[i][j];
                *set = i;
                *slice = j;
            }
        }
    } 
#endif
}

void get_elements_set_rr(long int eviction_set_rr[CACHE_SET_SIZE], long int new_ev_set[CACHE_SET_SIZE * CACHE_SLICES], long int *target_address, int slice)
{
    int i;
    eviction_set_rr[0] = (long int)target_address;
    for (i = 0; i < CACHE_SET_SIZE - 1; i++)
    {
        eviction_set_rr[i + 1] = new_ev_set[slice * CACHE_SET_SIZE + i];
    }
}

void prepare_sets(long int eviction_set_rr[CACHE_SET_SIZE], long int *conflicting_address, int time_prime_limit)
{
    int i;
    flush_data((long *)eviction_set_rr[0]); //remove target
    lfence();
    long int *prime_address = (long int *)eviction_set_rr[1]; //Note that first one is the target
    /*Ensure data is in the cache*/
    i = 0;
    int t = probe_one_set(prime_address);
    lfence();
    while (t > time_prime_limit)
    {
        t = probe_one_set(prime_address);
        i++;
        if (i > 10)
        {
            break;
        }
    }
    lfence();
    /*Remove data from the cache*/
    for (i = 0; i < CACHE_SET_SIZE; i++)
    {
        flush_data((long *)eviction_set_rr[i]);
        lfence();
    }
    flush_data(conflicting_address); //Remove the last element from the eviction set
    lfence();
    /*Place data in the cache again*/
    for (i = 0; i < CACHE_SET_SIZE; i++)
    {
        //lfence();
        mem_access((long int *)eviction_set_rr[i]);
        lfence();
    }
}

void increase_eviction(long int candidates_set[], int num_candidates, long int ev_set[CACHE_SET_SIZE * CACHE_SLICES], long int new_ev_set[CACHE_SET_SIZE * 2], int slice, int time_limit)
{
    int i, j, k;
    for (i = 0; i < CACHE_SET_SIZE; ++i)
    {
        new_ev_set[i] = ev_set[slice * CACHE_SET_SIZE + i];
    }
#if SLICE_HASH_AVAILABLE
    j = 0;
    for (i = 0; i < num_candidates; ++i)
    {
        if (!(check_inside(candidates_set[i], ev_set, CACHE_SET_SIZE * CACHE_SLICES)))
        {
            uintptr_t phys_addr;
            int res = virt_to_phys(&phys_addr, getpid(), (uintptr_t)candidates_set[i]);
            if (res < 0)
            {
                printf("Error \n");
                return;
            }
            int s = addr2slice_linear(phys_addr, CACHE_SLICES);
            if (s == slice)
            {
                new_ev_set[CACHE_SET_SIZE + j] = candidates_set[i];
                j++;
                if (j == CACHE_SET_SIZE)
                    break;
            }
        }
    }
#else
    long int *prime_address = (long int *)ev_set[slice * CACHE_SET_SIZE];
    j = 0;
    int cont;
    for (i = 0; i < num_candidates; ++i)
    {
        if (!(check_inside(candidates_set[i], ev_set, CACHE_SET_SIZE * CACHE_SLICES)))
        {
            cont = 0;
            for (k = 0; k < 1000; ++k)
            {
                int t1 = access_timed((long int *)candidates_set[i]);
                lfence();
                probe_one_set(prime_address);
                lfence();
                t1 = access_timed((long int *)candidates_set[i]);
                lfence();
                if (t1 > time_limit)
                {
                    cont++;
                }
            }
            if (cont > 800)
            {
                new_ev_set[CACHE_SET_SIZE + j] = candidates_set[i];
                j++;
                if (j == CACHE_SET_SIZE)
                    break;
            }
        }
    }
#endif
}
