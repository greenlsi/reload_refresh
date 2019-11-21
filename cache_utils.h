#include "common.h" /* virtual to physical page conversion */
#include "cache_details.h" /*Cache architecture details*/

#define SLICE_HASH_AVAILABLE 0
#define SLICE_MASK_0 0x1b5f575440UL
#define SLICE_MASK_1 0x2eb5faa880UL
#define SLICE_MASK_2 0x3cccc93100UL

#define OFF 8 // For the probe array

#define mfence()  __asm__ volatile("mfence;"); 
#define lfence()  __asm__ volatile("lfence;");

/*Set generation functions*/
void generate_candidates_array(long int *base_address, long int candidates_set[], int num_candidates,int tar_set);
void flush_desired_set(long int address_array[], int len);
int check_inside(long int dato,long int array_datos[],int array_length);
void randomize_set(long int array[],int len);
int probe_candidate(int number_elements, long int array_set[], long int *dir_candidate, int time_main_memory);
int create_filtered_set(long int filtered_set[], long int original_set[], int len, int time_main_memory);
int create_eviction_set(long int eviction_set[], long int filtered_set[], int len_filtered_set, long int original_set[], int len_original_set, int time_main_memory);
void initialize_sets(long int eviction_set[], long int filtered_set[], int len_filtered_set, long int original_set[], int len_original_set, int time_main_memory);
void store_invariant_part(long int eviction_set[CACHE_SET_SIZE * CACHE_SLICES], long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES]);
void generate_new_eviction_set(int set, long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES],long int new_ev_set[CACHE_SET_SIZE * CACHE_SLICES]);
void write_linked_list(long int set_desired[CACHE_SET_SIZE * CACHE_SLICES]);
void profile_address(long int invariant_part[CACHE_SET_SIZE * CACHE_SLICES], long int new_ev_set[CACHE_SET_SIZE * CACHE_SLICES],long int *target_address,int *set, int *slice);
void get_elements_set_rr(long int eviction_set_min[CACHE_SET_SIZE], long int new_ev_set[CACHE_SET_SIZE * CACHE_SLICES], long int *target_address, int slice);
void prepare_sets(long int eviction_set_rr[CACHE_SET_SIZE], long int *conflicting_address,int time_prime_limit); //ONLY FOR R+R
void increase_eviction(long int candidates_set[], int num_candidates, long int ev_set[CACHE_SET_SIZE * CACHE_SLICES], long int new_ev_set[CACHE_SET_SIZE * 2], int slice, int time_limit);

/*Useful functions for attackss*/
unsigned long int timestamp(void);
int parity(uint64_t v);
int addr2slice_linear(uintptr_t addr, int slices);
long int mem_access(long int *v);
int access_timed(long int *pos_data);
int access_timed_full(long int *pos_data);
int access_timed_flush(long int *pos_data);
int access_timed_full_flush(long int *pos_data);
void flush_data(long int *pos_data);
int flush_timed (long int *pos_data);
int probe_one_set(long int *pos_data);
int fast_prime(long int ev_set[], int S, int C, int D);
/*Code for Reload+Refresh*/
int reload_step(long int *pos_data, long int *second_data, long int *first_el);
int refresh_step(long int *pos_data);

