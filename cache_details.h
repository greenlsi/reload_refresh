/*Cache and memory architecture details*/
#define CACHE_SIZE 6 //MB
#define CPU_CORES 4
#define CACHE_SET_SIZE 12 //Ways
#define CACHE_SLICES 8
#define SETS_PER_SLICE 1024
#define BITS_SET 10
#define BITS_LINE 6 //64 bytes per cache line

#define BITS_HUGEPAGE 21
#define BITS_NORMALPAGE 12

/*Util data based on the cache architecture*/
#define MASC_KNOWN_SET_BITS ((1ULL << (BITS_NORMALPAGE - BITS_LINE)) - 1ULL)
#define NEXT_SET_NP 1LL << (BITS_NORMALPAGE - BITS_LINE)
#define NEXT_SET (1ULL << (BITS_SET + BITS_LINE))
#define MASC_BITS_SET ((1ULL << BITS_SET) - 1ULL)
#define MASC_SET (NEXT_SET - 1ULL)
#define MASC_SET_LINE ((1ULL << (BITS_SET + BITS_LINE)) - 1ULL)
#define MASC_SET_LINE_INV ~MASC_SET_LINE
