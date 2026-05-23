#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAMES 64
#define MAX_REFS 512

#define C_RESET "\033[0m"
#define C_RED "\033[31m"
#define C_GREEN "\033[32m"
#define C_CYAN "\033[36m"
#define C_BOLD "\033[1m"
#define C_YELLOW "\033[33m"

typedef struct {
  int page;
  int loaded;
  int ref_bit;
} Frame;

typedef struct {
  int hits;
  int misses;
  char name[32];
} Stats;

static void print_frames(Frame *frames, int n) {
  printf("[ ");
  for (int i = 0; i < n; i++) {
    if (frames[i].page == -1)
      printf(" _");
    else
      printf("%2d", frames[i].page);
    if (i < n - 1)
      printf(", ");
  }
  printf(" ]");
}

static int find_in_frames(Frame *frames, int n, int page) {
  for (int i = 0; i < n; i++)
    if (frames[i].page == page)
      return i;
  return -1;
}

static int find_empty(Frame *frames, int n) {
  for (int i = 0; i < n; i++)
    if (frames[i].page == -1)
      return i;
  return -1;
}

static void print_header(const char *algo_name, int n) {
  printf("\n" C_BOLD C_CYAN
         "══════════════════════════════════════════════════\n"
         "  %s  (N = %d)\n"
         "══════════════════════════════════════════════════\n" C_RESET,
         algo_name, n);
  printf("%-6s %-5s %-6s  %-*s  %s\n", "Step", "Page", "Result", n * 4 + 4,
         "Frames", "Evicted");
  printf("%-6s %-5s %-6s  %-*s  %s\n", "----", "----", "------", n * 4 + 4,
         "------", "-------");
}

Stats simulate_fifo(int *refs, int len, int n, int verbose) {
  Frame frames[MAX_FRAMES];
  Stats s = {0, 0, "FIFO"};
  for (int i = 0; i < n; i++) {
    frames[i].page = -1;
    frames[i].loaded = 0;
  }

  if (verbose)
    print_header("FIFO", n);

  for (int step = 0; step < len; step++) {
    int page = refs[step];
    int idx = find_in_frames(frames, n, page);
    int evicted = -1;

    if (idx != -1) {
      s.hits++;
      if (verbose) {
        printf("%-6d %-5d " C_GREEN "HIT   " C_RESET "  ", step + 1, page);
        print_frames(frames, n);
        printf("\n");
      }
    } else {
      s.misses++;
      int free_slot = find_empty(frames, n);
      if (free_slot != -1) {
        frames[free_slot].page = page;
        frames[free_slot].loaded = step;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  (load into free slot)\n");
        }
      } else {
        int oldest = 0;
        for (int i = 1; i < n; i++)
          if (frames[i].loaded < frames[oldest].loaded ||
              (frames[i].loaded == frames[oldest].loaded &&
               frames[i].page < frames[oldest].page))
            oldest = i;
        evicted = frames[oldest].page;
        frames[oldest].page = page;
        frames[oldest].loaded = step;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  evict→%d\n", evicted);
        }
      }
    }
  }

  if (verbose)
    printf(C_BOLD "Totals: hits=%d  misses=%d  hit-rate=%.2f%%\n" C_RESET,
           s.hits, s.misses, 100.0 * s.hits / (s.hits + s.misses));
  return s;
}

static int next_use(int *refs, int len, int from, int page) {
  for (int i = from; i < len; i++)
    if (refs[i] == page)
      return i;
  return INT_MAX;
}

Stats simulate_min(int *refs, int len, int n, int verbose) {
  Frame frames[MAX_FRAMES];
  Stats s = {0, 0, "MIN (Optimal)"};
  for (int i = 0; i < n; i++)
    frames[i].page = -1;

  if (verbose)
    print_header("MIN / Optimal", n);

  for (int step = 0; step < len; step++) {
    int page = refs[step];
    int idx = find_in_frames(frames, n, page);
    int evicted = -1;

    if (idx != -1) {
      s.hits++;
      if (verbose) {
        printf("%-6d %-5d " C_GREEN "HIT   " C_RESET "  ", step + 1, page);
        print_frames(frames, n);
        printf("\n");
      }
    } else {
      s.misses++;
      int free_slot = find_empty(frames, n);
      if (free_slot != -1) {
        frames[free_slot].page = page;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  (load into free slot)\n");
        }
      } else {
        int worst = 0;
        int worst_next = next_use(refs, len, step + 1, frames[0].page);
        for (int i = 1; i < n; i++) {
          int nu = next_use(refs, len, step + 1, frames[i].page);
          if (nu > worst_next ||
              (nu == worst_next && frames[i].page < frames[worst].page)) {
            worst = i;
            worst_next = nu;
          }
        }
        evicted = frames[worst].page;
        frames[worst].page = page;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  evict→%d", evicted);
          if (worst_next == INT_MAX)
            printf(" (never used again)");
          printf("\n");
        }
      }
    }
  }

  if (verbose)
    printf(C_BOLD "Totals: hits=%d  misses=%d  hit-rate=%.2f%%\n" C_RESET,
           s.hits, s.misses, 100.0 * s.hits / (s.hits + s.misses));
  return s;
}

Stats simulate_lru(int *refs, int len, int n, int verbose) {
  Frame frames[MAX_FRAMES];
  int last_use[MAX_FRAMES];
  Stats s = {0, 0, "LRU"};
  for (int i = 0; i < n; i++) {
    frames[i].page = -1;
    last_use[i] = -1;
  }

  if (verbose)
    print_header("LRU", n);

  for (int step = 0; step < len; step++) {
    int page = refs[step];
    int idx = find_in_frames(frames, n, page);
    int evicted = -1;

    if (idx != -1) {
      s.hits++;
      last_use[idx] = step;
      if (verbose) {
        printf("%-6d %-5d " C_GREEN "HIT   " C_RESET "  ", step + 1, page);
        print_frames(frames, n);
        printf("\n");
      }
    } else {
      s.misses++;
      int free_slot = find_empty(frames, n);
      if (free_slot != -1) {
        frames[free_slot].page = page;
        last_use[free_slot] = step;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  (load into free slot)\n");
        }
      } else {
        int lru = 0;
        for (int i = 1; i < n; i++) {
          if (last_use[i] < last_use[lru] ||
              (last_use[i] == last_use[lru] &&
               frames[i].page < frames[lru].page))
            lru = i;
        }
        evicted = frames[lru].page;
        frames[lru].page = page;
        last_use[lru] = step;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  evict→%d (last used at step %d)\n", evicted,
                 last_use[lru] - 1);
        }
      }
    }
  }

  if (verbose)
    printf(C_BOLD "Totals: hits=%d  misses=%d  hit-rate=%.2f%%\n" C_RESET,
           s.hits, s.misses, 100.0 * s.hits / (s.hits + s.misses));
  return s;
}

Stats simulate_second_chance(int *refs, int len, int n, int verbose) {
  Frame frames[MAX_FRAMES];
  int clock_hand = 0;
  Stats s = {0, 0, "Second Chance"};
  for (int i = 0; i < n; i++) {
    frames[i].page = -1;
    frames[i].ref_bit = 0;
    frames[i].loaded = 0;
  }

  if (verbose)
    print_header("Second Chance (Clock)", n);

  for (int step = 0; step < len; step++) {
    int page = refs[step];
    int idx = find_in_frames(frames, n, page);
    int evicted = -1;

    if (idx != -1) {
      s.hits++;
      frames[idx].ref_bit = 1;
      if (verbose) {
        printf("%-6d %-5d " C_GREEN "HIT   " C_RESET "  ", step + 1, page);
        print_frames(frames, n);
        printf("  (ref_bit[%d]=1)\n", idx);
      }
    } else {
      s.misses++;
      int free_slot = find_empty(frames, n);
      if (free_slot != -1) {
        frames[free_slot].page = page;
        frames[free_slot].ref_bit = 0;
        frames[free_slot].loaded = step;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  (load into free slot)\n");
        }
      } else {
        while (frames[clock_hand].ref_bit == 1) {
          frames[clock_hand].ref_bit = 0;
          clock_hand = (clock_hand + 1) % n;
        }
        evicted = frames[clock_hand].page;
        frames[clock_hand].page = page;
        frames[clock_hand].ref_bit = 0;
        frames[clock_hand].loaded = step;
        clock_hand = (clock_hand + 1) % n;
        if (verbose) {
          printf("%-6d %-5d " C_RED "MISS  " C_RESET "  ", step + 1, page);
          print_frames(frames, n);
          printf("  evict→%d\n", evicted);
        }
      }
    }
  }

  if (verbose)
    printf(C_BOLD "Totals: hits=%d  misses=%d  hit-rate=%.2f%%\n" C_RESET,
           s.hits, s.misses, 100.0 * s.hits / (s.hits + s.misses));
  return s;
}

static void print_summary(Stats *results, int count) {
  printf("\n" C_BOLD C_YELLOW
         "╔══════════════════════╦════════╦════════╦══════════╗\n"
         "║ Algorithm            ║  Hits  ║ Misses ║ Hit Rate ║\n"
         "╠══════════════════════╬════════╬════════╬══════════╣\n" C_RESET);
  for (int i = 0; i < count; i++) {
    int total = results[i].hits + results[i].misses;
    printf(C_BOLD C_YELLOW "║" C_RESET " %-20s " C_BOLD C_YELLOW "║" C_RESET
                           " %6d " C_BOLD C_YELLOW "║" C_RESET
                           " %6d " C_BOLD C_YELLOW "║" C_RESET
                           "  %5.2f%%  " C_BOLD C_YELLOW "║\n" C_RESET,
           results[i].name, results[i].hits, results[i].misses,
           100.0 * results[i].hits / total);
  }
  printf(C_BOLD C_YELLOW
         "╚══════════════════════╩════════╩════════╩══════════╝\n" C_RESET);
}

static void belady_demo(void) {
  int refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
  int len = 12;

  printf("\n" C_BOLD C_CYAN
         "══════════════════════════════════════════════════════\n"
         "  BELADY'S ANOMALY DEMO\n"
         "  Reference string: 1 2 3 4 1 2 5 1 2 3 4 5\n"
         "══════════════════════════════════════════════════════\n" C_RESET);
  printf("Belady's Anomaly: FIFO can have MORE page faults\n"
         "when the number of frames INCREASES.\n\n");

  Stats r3 = simulate_fifo(refs, len, 3, 0);
  Stats r4 = simulate_fifo(refs, len, 4, 0);

  printf("  FIFO with N=3: hits=%-3d misses=%-3d\n", r3.hits, r3.misses);
  printf("  FIFO with N=4: hits=%-3d misses=%-3d\n", r4.hits, r4.misses);

  if (r4.misses > r3.misses)
    printf(C_BOLD C_RED "\n  !! ANOMALY CONFIRMED: N=4 produces MORE misses "
                        "than N=3 !!\n" C_RESET);
  else
    printf("  (No anomaly on this run)\n");

  printf("\nStep-by-step with N=3:\n");
  simulate_fifo(refs, len, 3, 1);
  printf("\nStep-by-step with N=4:\n");
  simulate_fifo(refs, len, 4, 1);
}

static int parse_refs(char **argv, int start, int argc, int *refs, int *len) {
  *len = 0;
  for (int i = start; i < argc; i++) {
    char *end;
    long v = strtol(argv[i], &end, 10);
    if (*end != '\0') {
      fprintf(stderr, "Error: non-numeric token '%s'\n", argv[i]);
      return -1;
    }
    if (v < 0) {
      fprintf(stderr, "Error: negative page id %ld not allowed\n", v);
      return -1;
    }
    if (*len >= MAX_REFS) {
      fprintf(stderr, "Error: reference string too long (max %d)\n", MAX_REFS);
      return -1;
    }
    refs[(*len)++] = (int)v;
  }
  if (*len == 0) {
    fprintf(stderr, "Error: reference string is empty\n");
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 2 && strcmp(argv[1], "--belady") == 0) {
    belady_demo();
    return 0;
  }

  int refs_default[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3,
                        0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
  int n_default = 3;
  int refs[MAX_REFS];
  int len, n;

  if (argc == 1) {
    len = (int)(sizeof(refs_default) / sizeof(refs_default[0]));
    memcpy(refs, refs_default, len * sizeof(int));
    n = n_default;
    printf(C_BOLD "Using required reference dataset (N=%d):\n" C_RESET
                  "  7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1\n",
           n);
  } else if (argc >= 3) {
    char *end;
    n = (int)strtol(argv[1], &end, 10);
    if (*end != '\0' || n < 1) {
      fprintf(stderr, "Error: N must be an integer >= 1\n");
      return 1;
    }
    if (n > MAX_FRAMES) {
      fprintf(stderr, "Error: N too large (max %d)\n", MAX_FRAMES);
      return 1;
    }
    if (parse_refs(argv, 2, argc, refs, &len) != 0)
      return 1;
    printf(C_BOLD "Custom input: N=%d, %d references\n" C_RESET, n, len);
  } else {
    fprintf(stderr,
            "Usage:\n"
            "  %s                       (required dataset)\n"
            "  %s <N> <p1 p2 p3 ...>    (custom input)\n"
            "  %s --belady               (Belady's anomaly demo)\n",
            argv[0], argv[0], argv[0]);
    return 1;
  }

  Stats results[4];
  results[0] = simulate_fifo(refs, len, n, 1);
  results[1] = simulate_min(refs, len, n, 1);
  results[2] = simulate_lru(refs, len, n, 1);
  results[3] = simulate_second_chance(refs, len, n, 1);

  printf("\n" C_BOLD "COMPARISON SUMMARY\n" C_RESET);
  print_summary(results, 4);

  printf(
      "\n" C_BOLD "Tie-break rules used:\n" C_RESET
      "  MIN:           smallest page id among tied candidates\n"
      "  LRU:           smallest page id among tied last-use times\n"
      "  Second Chance: oldest-loaded page when multiple have ref_bit=0\n\n");

  printf("Run with --belady to see Belady's Anomaly demonstration.\n\n");
  return 0;
}
