#define _XOPEN_SOURCE 700
#include <fcntl.h>  
#include <math.h>
#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h>   
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define MASC_PAGE (PAGE_SIZE - 1)
#define BITS_PAGE (int)log2(PAGE_SIZE)

typedef struct
{
    uint64_t pagefn : 54;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
} PagemapEntry;


int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr);
int virt_to_phys(uintptr_t *paddr, pid_t pid, uintptr_t vaddr);
