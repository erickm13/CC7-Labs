#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_THREADS 15
#define MIN_THREADS 5
#define TIME_QUANTUM 2

typedef struct {
  int id;
  int burst_time;
  int arrival_time;
  int remaining_time;
  int waiting_time;
  int turnaround_time;
  int completion_time;
  int start_time;
} Process;

int num_processes;
Process origin[MAX_THREADS];
Process processes[MAX_THREADS];
double g_avg_wait[4], g_avg_turn[4];

void get_timestamp(char *buf, int sim_time) {
  time_t base;
  time(&base);
  base += sim_time;
  struct tm *t = localtime(&base);
  strftime(buf, 64, "%a %b %d %H:%M:%S %Y", t);
}

void clean_processes() {
  for (int i = 0; i < num_processes; i++) {
    processes[i] = origin[i];
    processes[i].remaining_time = processes[i].burst_time;
    processes[i].waiting_time = 0;
    processes[i].turnaround_time = 0;
    processes[i].completion_time = 0;
    processes[i].start_time = -1;
  }
}

int order_arrival(const void *a, const void *b) {
  return ((Process *)a)->arrival_time - ((Process *)b)->arrival_time;
}

void avg_calculate(int n, int algo_idx) {
  double avg_w = 0, avg_t = 0;
  for (int i = 0; i < n; i++) {
    avg_w += processes[i].waiting_time;
    avg_t += processes[i].turnaround_time;
  }
  avg_w /= n;
  avg_t /= n;
  g_avg_wait[algo_idx] = avg_w;
  g_avg_turn[algo_idx] = avg_t;
  printf("Avg Waiting Time: %.2f seconds\n", avg_w);
  printf("Avg Turnaround Time: %.2f seconds\n", avg_t);
}

/* ── FIFO ── */
void run_fifo() {
  clean_processes();
  qsort(processes, num_processes, sizeof(Process), order_arrival);
  printf("\n--- FIFO Scheduling ---\n");

  int current_time = 0;
  char ts[64];

  for (int i = 0; i < num_processes; i++) {
    if (current_time < processes[i].arrival_time)
      current_time = processes[i].arrival_time;

    processes[i].waiting_time = current_time - processes[i].arrival_time;
    processes[i].start_time = current_time;
    current_time += processes[i].burst_time;
    processes[i].completion_time = current_time;
    processes[i].turnaround_time = current_time - processes[i].arrival_time;

    if (processes[i].waiting_time == 0) {
      /* llegó justo cuando el CPU estaba libre */
      get_timestamp(ts, processes[i].arrival_time);
      printf("[%s] Process %d (Burst %d): Arrived\n", ts, processes[i].id,
             processes[i].burst_time);
      get_timestamp(ts, processes[i].start_time);
      printf("[%s] Process %d (Burst %d): Started\n", ts, processes[i].id,
             processes[i].burst_time);
    } else {
      /* llegó antes pero tuvo que esperar: una sola línea */
      get_timestamp(ts, processes[i].start_time);
      printf("[%s] Process %d (Burst %d): Arrived at %d, Started (waited %.2f "
             "seconds)\n",
             ts, processes[i].id, processes[i].burst_time,
             processes[i].arrival_time, (double)processes[i].waiting_time);
    }

    get_timestamp(ts, processes[i].completion_time);
    printf("[%s] Process %d (Burst %d): Completed\n", ts, processes[i].id,
           processes[i].burst_time);
  }

  printf("\nWaiting Times:    [");
  for (int i = 0; i < num_processes; i++)
    printf("%.2f%s", (double)processes[i].waiting_time,
           i < num_processes - 1 ? ", " : "]\n");
  printf("Turnaround Times: [");
  for (int i = 0; i < num_processes; i++)
    printf("%.2f%s", (double)processes[i].turnaround_time,
           i < num_processes - 1 ? ", " : "]\n");
  avg_calculate(num_processes, 0);
}

/* ── Round Robin ── */
void run_round_robin() {
  clean_processes();
  printf("\n--- Round Robin Scheduling (Quantum %d) ---\n", TIME_QUANTUM);

  int current_time = 0;
  int completed = 0;
  int queue[MAX_THREADS * 100];
  int q_front = 0, q_rear = 0;
  int in_queue[MAX_THREADS] = {0};
  char ts[64];

  for (int i = 0; i < num_processes; i++) {
    if (processes[i].arrival_time == 0) {
      queue[q_rear++] = i;
      in_queue[i] = 1;
    }
  }

  while (completed < num_processes) {
    if (q_front == q_rear) {
      current_time++;
      for (int i = 0; i < num_processes; i++) {
        if (!in_queue[i] && processes[i].arrival_time <= current_time &&
            processes[i].remaining_time > 0) {
          queue[q_rear++] = i;
          in_queue[i] = 1;
        }
      }
      continue;
    }

    int idx = queue[q_front++];
    in_queue[idx] = 0;

    /* primera vez que corre este proceso */
    if (processes[idx].start_time == -1) {
      processes[idx].start_time = current_time;
      double waited = current_time - processes[idx].arrival_time;
      get_timestamp(ts, current_time);
      if (waited > 0) {
        printf("[%s] Process %d (Burst %d): Arrived at %d, Started (waited "
               "%.2f seconds)\n",
               ts, processes[idx].id, processes[idx].burst_time,
               processes[idx].arrival_time, waited);
      } else {
        get_timestamp(ts, processes[idx].arrival_time);
        printf("[%s] Process %d (Burst %d): Arrived\n", ts, processes[idx].id,
               processes[idx].burst_time);
        get_timestamp(ts, current_time);
        printf("[%s] Process %d (Burst %d): Started\n", ts, processes[idx].id,
               processes[idx].burst_time);
      }
    }

    int runtime = (processes[idx].remaining_time < TIME_QUANTUM)
                      ? processes[idx].remaining_time
                      : TIME_QUANTUM;
    processes[idx].remaining_time -= runtime;
    current_time += runtime;

    for (int i = 0; i < num_processes; i++) {
      if (processes[i].arrival_time <= current_time && !in_queue[i] &&
          processes[i].remaining_time > 0 && i != idx) {
        queue[q_rear++] = i;
        in_queue[i] = 1;
      }
    }

    if (processes[idx].remaining_time > 0) {
      get_timestamp(ts, current_time);
      printf("[%s] Process %d (Burst %d remaining): Preempted\n", ts,
             processes[idx].id, processes[idx].remaining_time);
      queue[q_rear++] = idx;
      in_queue[idx] = 1;
    } else {
      processes[idx].completion_time = current_time;
      processes[idx].turnaround_time =
          current_time - processes[idx].arrival_time;
      processes[idx].waiting_time =
          processes[idx].turnaround_time - processes[idx].burst_time;
      get_timestamp(ts, current_time);
      printf("[%s] Process %d (Burst %d): Completed\n", ts, processes[idx].id,
             processes[idx].burst_time);
      completed++;
    }
  }
  avg_calculate(num_processes, 1);
}

/* ── SJF ── */
void run_sjf() {
  clean_processes();
  printf("\n--- SJF Scheduling ---\n");

  int current_time = 0;
  int completed = 0;
  int done[MAX_THREADS] = {0};
  char ts[64];

  while (completed < num_processes) {
    int best = -1;
    for (int i = 0; i < num_processes; i++) {
      if (!done[i] && processes[i].arrival_time <= current_time) {
        if (best == -1 || processes[i].burst_time < processes[best].burst_time)
          best = i;
      }
    }
    if (best == -1) {
      current_time++;
      continue;
    }

    processes[best].waiting_time = current_time - processes[best].arrival_time;
    processes[best].start_time = current_time;
    current_time += processes[best].burst_time;
    processes[best].completion_time = current_time;
    processes[best].turnaround_time =
        current_time - processes[best].arrival_time;
    done[best] = 1;
    completed++;

    if (processes[best].waiting_time == 0) {
      get_timestamp(ts, processes[best].arrival_time);
      printf("[%s] Process %d (Burst %d): Arrived\n", ts, processes[best].id,
             processes[best].burst_time);
      get_timestamp(ts, processes[best].start_time);
      printf("[%s] Process %d (Burst %d): Started\n", ts, processes[best].id,
             processes[best].burst_time);
    } else {
      get_timestamp(ts, processes[best].start_time);
      printf("[%s] Process %d (Burst %d): Arrived at %d, Started (waited %.2f "
             "seconds)\n",
             ts, processes[best].id, processes[best].burst_time,
             processes[best].arrival_time,
             (double)processes[best].waiting_time);
    }

    get_timestamp(ts, processes[best].completion_time);
    printf("[%s] Process %d (Burst %d): Completed\n", ts, processes[best].id,
           processes[best].burst_time);
  }
  avg_calculate(num_processes, 2);
}

/* ── SRTF ── */
void run_srtf() {
  clean_processes();
  printf("\n--- SRTF Scheduling ---\n");

  int current_time = 0;
  int completed = 0;
  int prev = -1;
  char ts[64];

  while (completed < num_processes) {
    int best = -1;
    for (int i = 0; i < num_processes; i++) {
      if (processes[i].arrival_time <= current_time &&
          processes[i].remaining_time > 0) {
        if (best == -1 ||
            processes[i].remaining_time < processes[best].remaining_time)
          best = i;
      }
    }
    if (best == -1) {
      current_time++;
      continue;
    }

    if (prev != best) {
      get_timestamp(ts, current_time);

      /* preempt el proceso anterior */
      if (prev != -1 && processes[prev].remaining_time > 0)
        printf("[%s] Process %d (Burst %d): Arrived at %d, Preempted %d "
               "(remaining %d)\n",
               ts, processes[best].id, processes[best].burst_time,
               processes[best].arrival_time, prev,
               processes[prev].remaining_time);

      /* start o resume del nuevo */
      if (processes[best].start_time == -1) {
        processes[best].start_time = current_time;
        printf("[%s] Process %d (Burst %d): Arrived\n", ts, processes[best].id,
               processes[best].burst_time);
        printf("[%s] Process %d (Burst %d): Started\n", ts, processes[best].id,
               processes[best].burst_time);
      }
      prev = best;
    }

    processes[best].remaining_time--;
    current_time++;

    if (processes[best].remaining_time == 0) {
      processes[best].completion_time = current_time;
      processes[best].turnaround_time =
          current_time - processes[best].arrival_time;
      processes[best].waiting_time =
          processes[best].turnaround_time - processes[best].burst_time;
      get_timestamp(ts, current_time);
      printf("[%s] Process %d (Burst %d): Completed\n", ts, processes[best].id,
             processes[best].burst_time);
      completed++;
      prev = -1;
    }
  }
  avg_calculate(num_processes, 3);
}

/* ── main ── */
int main() {
  srand(time(NULL));

  num_processes = (rand() % (MAX_THREADS - MIN_THREADS + 1)) + MIN_THREADS;
  for (int i = 0; i < num_processes; i++) {
    origin[i].id = i;
    origin[i].burst_time = (rand() % 10) + 1;
    origin[i].arrival_time = rand() % 21;
  }

  printf("Dataset: %d threads\n", num_processes);
  printf("Burst Times:   [");
  for (int i = 0; i < num_processes; i++)
    printf("%d%s", origin[i].burst_time, i < num_processes - 1 ? ", " : "]\n");
  printf("Arrival Times: [");
  for (int i = 0; i < num_processes; i++)
    printf("%d%s", origin[i].arrival_time,
           i < num_processes - 1 ? ", " : "]\n");

  run_fifo();
  run_round_robin();
  run_sjf();
  run_srtf();

  const char *labels[] = {"FIFO", "RR(q=2)", "SJF", "SRTF"};
  printf("\n===========================================\n");
  printf("         COMPARISON SUMMARY\n");
  printf("===========================================\n");
  printf("%-10s %-22s %-22s\n", "Algorithm", "Avg Waiting Time",
         "Avg Turnaround Time");
  printf("%-10s %-22s %-22s\n", "----------", "----------------",
         "-------------------");
  for (int i = 0; i < 4; i++) {
    char w[32], t[32];
    snprintf(w, sizeof(w), "%.2f seconds", g_avg_wait[i]);
    snprintf(t, sizeof(t), "%.2f seconds", g_avg_turn[i]);
    printf("%-10s %-22s %-22s\n", labels[i], w, t);
  }
  printf("\n");
  return 0;
}
