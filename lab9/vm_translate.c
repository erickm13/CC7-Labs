#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_FRAMES  100
#define PAGE_SIZE   256
#define MAX_VA      0xFFFFUL
#define MAX_VPN     256

/* ---------- data structures ---------- */

typedef struct { int valid; int pfn; } PTE;
typedef struct { int occupied; }       Frame;

static Frame ram[NUM_FRAMES];
static PTE   page_table[MAX_VPN];

/* ---------- frame allocator ---------- */

static int count_free(void) {
    int n = 0;
    for (int i = 0; i < NUM_FRAMES; i++)
        if (!ram[i].occupied) n++;
    return n;
}

/* Returns index of first FREE frame and marks it OCCUPIED; -1 if none. */
static int allocate_frame(void) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (!ram[i].occupied) {
            ram[i].occupied = 1;
            return i;
        }
    }
    return -1;
}

/* ---------- RAM init ----------
   Randomly occupies [10,60] frames, retrying until FREE >= max(10,v).
   Capped at 100 attempts; after that forces a safe fallback layout.        */
static void init_ram(int v, unsigned int seed) {
    srand(seed);
    int min_free = v > 10 ? v : 10;
    int attempts = 0;

    while (1) {
        for (int i = 0; i < NUM_FRAMES; i++) ram[i].occupied = 0;

        int occ_target = 10 + rand() % 51;   /* [10, 60] */
        int marked = 0;
        while (marked < occ_target) {
            int idx = rand() % NUM_FRAMES;
            if (!ram[idx].occupied) { ram[idx].occupied = 1; marked++; }
        }

        if (count_free() >= min_free) break;

        if (++attempts >= 100) {
            /* Fallback: occupy exactly 10 frames (indices 0..9). */
            for (int i = 0; i < NUM_FRAMES; i++) ram[i].occupied = 0;
            for (int i = 0; i < 10; i++)         ram[i].occupied = 1;
            break;
        }
    }
}

/* ---------- RAM printout ---------- */

static void print_ram(unsigned int seed) {
    int free_cnt = count_free();
    printf("PHYSICAL RAM (100 frames) after random init (seed=%u):\n", seed);
    printf("FREE=%d OCCUPIED=%d\n", free_cnt, NUM_FRAMES - free_cnt);
    for (int i = 0; i < NUM_FRAMES; i++) {
        printf("%2d:%c ", i, ram[i].occupied ? 'X' : 'F');
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

/* ---------- process load ---------- */

static int load_process(int v) {
    for (int i = 0; i < v; i++) { page_table[i].valid = 0; page_table[i].pfn = -1; }

    printf("Load process: V=%d -> VPN 0..%d mapped to PFNs [", v, v - 1);
    for (int i = 0; i < v; i++) {
        int frame = allocate_frame();
        if (frame == -1) {
            printf(" ]\nERROR: out of frames at VPN %d — aborting load\n", i);
            return -1;
        }
        page_table[i].valid = 1;
        page_table[i].pfn   = frame;
        printf("%s%d", i ? ", " : " ", frame);
    }
    printf(" ]\n\n");
    return 0;
}

/* ---------- address translation ---------- */

static void translate(unsigned long va, int v) {
    if (va > MAX_VA) {
        printf("VA=%-6lu                          ERROR=VA_OUT_OF_RANGE\n", va);
        return;
    }

    unsigned int vpn    = (unsigned int)((va >> 8) & 0xFF);
    unsigned int offset = (unsigned int)(va & 0xFF);

    if ((int)vpn >= v) {
        printf("VA=0x%04lX (%-5lu) VPN=0x%02X OFF=0x%02X  ERROR=VPN_OUT_OF_RANGE (vpn=%u, V=%d)\n",
               va, va, vpn, offset, vpn, v);
        return;
    }

    if (!page_table[vpn].valid) {
        printf("VA=0x%04lX (%-5lu) VPN=0x%02X OFF=0x%02X  ERROR=PAGE_NOT_MAPPED\n",
               va, va, vpn, offset);
        return;
    }

    int           pfn = page_table[vpn].pfn;
    unsigned long pa  = (unsigned long)pfn * PAGE_SIZE + offset;
    printf("VA=0x%04lX (%-5lu) VPN=0x%02X OFF=0x%02X  PFN=%-3d PA=%lu\n",
           va, va, vpn, offset, pfn, pa);
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_virtual_pages> <address_file> [seed]\n", argv[0]);
        return 1;
    }

    int v = atoi(argv[1]);
    if (v < 1 || v > 256) {
        fprintf(stderr, "ERROR: num_virtual_pages must be 1..256\n");
        return 1;
    }

    unsigned int seed = (argc >= 4) ? (unsigned int)atoi(argv[3])
                                    : (unsigned int)time(NULL);

    /* Task 3 — RAM init + printout */
    init_ram(v, seed);
    print_ram(seed);

    /* Task 3 — load process */
    if (load_process(v) != 0) return 1;

    /* Task 4 — batch translation */
    printf("--- Address Translation ---\n");

    FILE *f = fopen(argv[2], "r");
    if (!f) {
        fprintf(stderr, "ERROR: cannot open '%s'\n", argv[2]);
        return 1;
    }

    char line[64];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;

        unsigned long va;
        if (strncmp(line, "0x", 2) == 0 || strncmp(line, "0X", 2) == 0)
            va = strtoul(line + 2, NULL, 16);
        else
            va = strtoul(line, NULL, 10);

        translate(va, v);
    }

    fclose(f);
    return 0;
}
