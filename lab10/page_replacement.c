#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAMES 64
#define MAX_REFS 512

#define C_RESET "\033[0m"
#define C_RED "\033[31m"
#define C_GREEN "\033[32m"
#define C_YELLOW "\033[33m"
#define C_CYAN "\033[36m"
#define C_BOLD "\033[1m"
#define C_DIM "\033[2m"

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

static int find_in_frames(Frame *f, int n, int page) {
  for (int i = 0; i < n; i++)
    if (f[i].page == page)
      return i;
  return -1;
}
static int find_empty(Frame *f, int n) {
  for (int i = 0; i < n; i++)
    if (f[i].page == -1)
      return i;
  return -1;
}
static int next_use(int *refs, int len, int from, int page) {
  for (int i = from; i < len; i++)
    if (refs[i] == page)
      return i;
  return INT_MAX;
}

typedef struct {
  int snapshots[MAX_REFS][MAX_FRAMES];
  int filled[MAX_REFS];
  char result[MAX_REFS];
  int hits, misses;
  char name[32];
} AlgoResult;

AlgoResult run_fifo(int *refs, int len, int n) {
  AlgoResult r = {0};
  strcpy(r.name, "FIFO");
  Frame frames[MAX_FRAMES];
  for (int i = 0; i < n; i++) {
    frames[i].page = -1;
    frames[i].loaded = 0;
  }
  for (int s = 0; s < len; s++) {
    int page = refs[s];
    int idx = find_in_frames(frames, n, page);
    if (idx != -1) {
      r.result[s] = 'H';
      r.hits++;
    } else {
      r.result[s] = 'M';
      r.misses++;
      int fs = find_empty(frames, n);
      if (fs != -1) {
        frames[fs].page = page;
        frames[fs].loaded = s;
      } else {
        int old = 0;
        for (int i = 1; i < n; i++)
          if (frames[i].loaded < frames[old].loaded)
            old = i;
        frames[old].page = page;
        frames[old].loaded = s;
      }
    }
    for (int i = 0; i < n; i++)
      r.snapshots[s][i] = frames[i].page;
    r.filled[s] = n;
  }
  return r;
}

AlgoResult run_lru(int *refs, int len, int n) {
  AlgoResult r = {0};
  strcpy(r.name, "LRU");
  Frame frames[MAX_FRAMES];
  int last[MAX_FRAMES];
  for (int i = 0; i < n; i++) {
    frames[i].page = -1;
    last[i] = -1;
  }
  for (int s = 0; s < len; s++) {
    int page = refs[s];
    int idx = find_in_frames(frames, n, page);
    if (idx != -1) {
      r.result[s] = 'H';
      r.hits++;
      last[idx] = s;
    } else {
      r.result[s] = 'M';
      r.misses++;
      int fs = find_empty(frames, n);
      if (fs != -1) {
        frames[fs].page = page;
        last[fs] = s;
      } else {
        int lru = 0;
        for (int i = 1; i < n; i++)
          if (last[i] < last[lru] ||
              (last[i] == last[lru] && frames[i].page < frames[lru].page))
            lru = i;
        frames[lru].page = page;
        last[lru] = s;
      }
    }
    for (int i = 0; i < n; i++)
      r.snapshots[s][i] = frames[i].page;
    r.filled[s] = n;
  }
  return r;
}

AlgoResult run_min(int *refs, int len, int n) {
  AlgoResult r = {0};
  strcpy(r.name, "MIN");
  Frame frames[MAX_FRAMES];
  for (int i = 0; i < n; i++)
    frames[i].page = -1;
  for (int s = 0; s < len; s++) {
    int page = refs[s];
    int idx = find_in_frames(frames, n, page);
    if (idx != -1) {
      r.result[s] = 'H';
      r.hits++;
    } else {
      r.result[s] = 'M';
      r.misses++;
      int fs = find_empty(frames, n);
      if (fs != -1)
        frames[fs].page = page;
      else {
        int worst = 0, wn = next_use(refs, len, s + 1, frames[0].page);
        for (int i = 1; i < n; i++) {
          int nu = next_use(refs, len, s + 1, frames[i].page);
          if (nu > wn || (nu == wn && frames[i].page < frames[worst].page)) {
            worst = i;
            wn = nu;
          }
        }
        frames[worst].page = page;
      }
    }
    for (int i = 0; i < n; i++)
      r.snapshots[s][i] = frames[i].page;
    r.filled[s] = n;
  }
  return r;
}

AlgoResult run_sc(int *refs, int len, int n) {
  AlgoResult r = {0};
  strcpy(r.name, "2da Oport.");
  Frame frames[MAX_FRAMES];
  int hand = 0;
  for (int i = 0; i < n; i++) {
    frames[i].page = -1;
    frames[i].ref_bit = 0;
  }
  for (int s = 0; s < len; s++) {
    int page = refs[s];
    int idx = find_in_frames(frames, n, page);
    if (idx != -1) {
      r.result[s] = 'H';
      r.hits++;
      frames[idx].ref_bit = 1;
    } else {
      r.result[s] = 'M';
      r.misses++;
      int fs = find_empty(frames, n);
      if (fs != -1) {
        frames[fs].page = page;
        frames[fs].ref_bit = 0;
      } else {
        while (frames[hand].ref_bit == 1) {
          frames[hand].ref_bit = 0;
          hand = (hand + 1) % n;
        }
        frames[hand].page = page;
        frames[hand].ref_bit = 0;
        hand = (hand + 1) % n;
      }
    }
    for (int i = 0; i < n; i++)
      r.snapshots[s][i] = frames[i].page;
    r.filled[s] = n;
  }
  return r;
}

void print_table(AlgoResult *r, int *refs, int len, int n) {
  int cw = 4;

  printf(C_BOLD C_CYAN "  %s\n" C_RESET, r->name);

  printf(C_DIM "  Ref ");
  for (int s = 0; s < len; s++)
    printf("%*d", cw, refs[s]);
  printf("\n" C_RESET);

  printf(C_DIM "  ----");
  for (int s = 0; s < len; s++) {
    for (int k = 0; k < cw; k++)
      printf("-");
  }
  printf("\n" C_RESET);

  for (int f = 0; f < n; f++) {
    printf("  F%-2d ", f + 1);
    for (int s = 0; s < len; s++) {
      int pg = r->snapshots[s][f];
      if (pg == -1) {
        printf(C_DIM "%*s" C_RESET, cw, ".");
      } else {
        int is_new =
            (r->result[s] == 'M') && (s == 0 || r->snapshots[s - 1][f] != pg);
        if (is_new)
          printf(C_YELLOW "%*d" C_RESET, cw, pg);
        else
          printf("%*d", cw, pg);
      }
    }
    printf("\n");
  }

  printf(C_DIM "  ----");
  for (int s = 0; s < len; s++) {
    for (int k = 0; k < cw; k++)
      printf("-");
  }
  printf("\n" C_RESET);

  printf("       ");
  for (int s = 0; s < len; s++) {
    if (r->result[s] == 'H')
      printf(C_GREEN "%*c" C_RESET, cw, 'H');
    else
      printf(C_RED "%*c" C_RESET, cw, 'F');
  }
  printf("\n");

  int total = r->hits + r->misses;
  printf("  " C_BOLD "Hits: " C_GREEN "%d" C_RESET C_BOLD "  Fallos: " C_RED
         "%d" C_RESET C_BOLD "  Hit rate: " C_CYAN "%.1f%%" C_RESET "\n\n",
         r->hits, r->misses, 100.0 * r->hits / total);
}

void print_summary(AlgoResult *results, int count, int total) {
  printf(C_BOLD C_YELLOW
         "╔══════════════════╦════════╦════════╦══════════╗\n"
         "║ Algoritmo        ║  Hits  ║ Fallos ║ Hit Rate ║\n"
         "╠══════════════════╬════════╬════════╬══════════╣\n" C_RESET);
  for (int i = 0; i < count; i++) {
    printf(C_BOLD C_YELLOW "║" C_RESET " %-16s " C_BOLD C_YELLOW
                           "║" C_RESET C_GREEN " %6d " C_RESET C_BOLD C_YELLOW
                           "║" C_RESET C_RED " %6d " C_RESET C_BOLD C_YELLOW
                           "║" C_RESET C_CYAN
                           "  %5.1f%%  " C_RESET C_BOLD C_YELLOW "║\n" C_RESET,
           results[i].name, results[i].hits, results[i].misses,
           100.0 * results[i].hits / total);
  }
  printf(C_BOLD C_YELLOW
         "╚══════════════════╩════════╩════════╩══════════╝\n" C_RESET);
}

void belady_demo(void) {
  int refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
  int len = 12;
  printf(C_BOLD C_CYAN "\n=== Anomalía de Belady ===\n" C_RESET);
  printf("Secuencia: 1 2 3 4 1 2 5 1 2 3 4 5\n\n");
  AlgoResult r3 = run_fifo(refs, len, 3);
  AlgoResult r4 = run_fifo(refs, len, 4);
  printf("FIFO con N=3:\n");
  print_table(&r3, refs, len, 3);
  printf("FIFO con N=4:\n");
  print_table(&r4, refs, len, 4);
  if (r4.misses > r3.misses)
    printf(C_BOLD C_RED "!! ANOMALÍA CONFIRMADA: N=4 produce MÁS fallos que "
                        "N=3 !!\n\n" C_RESET);
}

static int parse_refs(char **argv, int start, int argc, int *refs, int *len) {
  *len = 0;
  for (int i = start; i < argc; i++) {
    char *end;
    long v = strtol(argv[i], &end, 10);
    if (*end != '\0') {
      fprintf(stderr, "Error: token no numérico '%s'\n", argv[i]);
      return -1;
    }
    if (v < 0) {
      fprintf(stderr, "Error: id de página negativo\n");
      return -1;
    }
    if (*len >= MAX_REFS) {
      fprintf(stderr, "Error: secuencia muy larga\n");
      return -1;
    }
    refs[(*len)++] = (int)v;
  }
  if (*len == 0) {
    fprintf(stderr, "Error: secuencia vacía\n");
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
  int refs[MAX_REFS], len, n;

  if (argc == 1) {
    len = sizeof(refs_default) / sizeof(refs_default[0]);
    memcpy(refs, refs_default, len * sizeof(int));
    n = n_default;
  } else if (argc >= 3) {
    char *end;
    n = (int)strtol(argv[1], &end, 10);
    if (*end != '\0' || n < 1 || n > MAX_FRAMES) {
      fprintf(stderr, "Error: N inválido\n");
      return 1;
    }
    if (parse_refs(argv, 2, argc, refs, &len) != 0)
      return 1;
  } else {
    fprintf(stderr, "Uso:\n  %s\n  %s <N> <p1 p2 ...>\n  %s --belady\n",
            argv[0], argv[0], argv[0]);
    return 1;
  }

  printf(C_BOLD "\nN = %d  |  Secuencia: ", n);
  for (int i = 0; i < len; i++)
    printf("%d%s", refs[i], i < len - 1 ? " " : "");
  printf("\n\n" C_RESET);

  AlgoResult results[4];
  results[0] = run_fifo(refs, len, n);
  results[1] = run_lru(refs, len, n);
  results[2] = run_min(refs, len, n);
  results[3] = run_sc(refs, len, n);

  for (int i = 0; i < 4; i++)
    print_table(&results[i], refs, len, n);

  printf(C_BOLD "Resumen\n" C_RESET);
  print_summary(results, 4, len);
  printf("\n");
  return 0;
}
