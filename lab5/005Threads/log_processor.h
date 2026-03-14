#ifndef LOG_PROCESSOR_H
#define LOG_PROCESSOR_H

#define MAX_LINE_LENGTH 512
#define MAX_COUNTERS 2048
#define MAX_KEY_LENGTH 128

typedef struct {
    char key[MAX_KEY_LENGTH];
    int count;
} Counter;

typedef struct {
    char **lines;
    int start;
    int end;

    Counter ip_counts[MAX_COUNTERS];
    int ip_size;

    Counter url_counts[MAX_COUNTERS];
    int url_size;

    int error_count;
} ThreadData;

char **read_log_file(const char *filename, int *line_count);
void free_log_lines(char **lines, int line_count);

int parse_log_line(const char *line, char *ip, char *url, int *status);

void increment_counter(Counter *arr, int *size, const char *key);
void merge_counters(Counter *dest, int *dest_size, Counter *src, int src_size);
int find_max_counter_index(Counter *arr, int size);

void *process_chunk(void *arg);

#endif
