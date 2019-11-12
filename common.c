#include "common.h" /* virtual to physical page conversion */

/*Returns 0 if successful, -1 on the contrary*/
int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr)
{
    int ret;
    uint64_t data;
    uintptr_t vpn;
    vpn = vaddr / PAGE_SIZE;
    ret = pread(pagemap_fd, &data, sizeof(data), vpn * sizeof(data));
    if (ret <= 0)
    {
        printf("Unable to read the pagemap file \n");
        return -1;
    }
    //printf("PAGE DATA %lx %lx\n",ret,data);
    entry->pagefn = data & ((1ULL << 54) - 1);
    entry->soft_dirty = (data >> 54) & 1;
    entry->file_page = (data >> 61) & 1;
    entry->swapped = (data >> 62) & 1;
    entry->present = (data >> 63) & 1;
    return 0;
}

/*Returns 0 if successful, -1 on the contrary*/
int virt_to_phys(uintptr_t *paddr, pid_t pid, uintptr_t vaddr)
{
    char pagemap_file[BUFSIZ];
    int pagemap_fd;

    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%u/pagemap", (unsigned int)pid);
    pagemap_fd = open(pagemap_file, O_RDONLY);
    //printf("ENTRY FD %i \n", pagemap_fd);
    if (pagemap_fd < 0)
    {
        printf("Unable to open the pagemap for the PID %u, check you have the proper access rights \n", pid);
        return -1;
    }
    PagemapEntry entry;
    if (pagemap_get_entry(&entry, pagemap_fd, vaddr) != 0)
    {
        return -1;
    }
    close(pagemap_fd);
    *paddr = ((entry.pagefn & ((1ULL << 54) - 1)) << BITS_PAGE) | (vaddr & MASC_PAGE);
    return 0;
}