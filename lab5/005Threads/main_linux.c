#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "log_processor.h"

int main(int argc, char *argv[]) {
    // validar argumentos
    if (argc < 3) {
        printf("Uso: %s <access.log> <num_threads>\n", argv[0]);
        return 1;
    }

    // guardar archivo y cantidad de threads
    const char *filename = argv[1];
    int num_threads = atoi(argv[2]);

    // leer todo el log
    int line_count = 0;
    char **lines = read_log_file(filename, &line_count);

    // validar lectura
    if (lines == NULL) {
        printf("Error al leer el archivo.\n");
        return 1;
    }

    // reservar memoria para threads y sus datos
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    ThreadData *thread_data = malloc(sizeof(ThreadData) * num_threads);

    // calcular cuantas lineas le tocan a cada thread
    int chunk_size = line_count / num_threads;
    int remainder = line_count % num_threads;
    int current_start = 0;

    // crear threads
    for (int i = 0; i < num_threads; i++) {
        // repartir sobrantes
        int extra = (i < remainder) ? 1 : 0;
        int current_end = current_start + chunk_size + extra;

        // asignar bloque de trabajo
        thread_data[i].lines = lines;
        thread_data[i].start = current_start;
        thread_data[i].end = current_end;

        // iniciar contadores locales
        thread_data[i].ip_size = 0;
        thread_data[i].url_size = 0;
        thread_data[i].error_count = 0;

        // lanzar thread
        pthread_create(&threads[i], NULL, process_chunk, &thread_data[i]);

        // mover inicio al siguiente bloque
        current_start = current_end;
    }

    // esperar a que todos terminen
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // contadores finales
    Counter final_ip_counts[MAX_COUNTERS];
    Counter final_url_counts[MAX_COUNTERS];
    int final_ip_size = 0;
    int final_url_size = 0;
    int total_errors = 0;

    // unir resultados de todos los threads
    for (int i = 0; i < num_threads; i++) {
        merge_counters(final_ip_counts, &final_ip_size,
                       thread_data[i].ip_counts, thread_data[i].ip_size);

        merge_counters(final_url_counts, &final_url_size,
                       thread_data[i].url_counts, thread_data[i].url_size);

        total_errors += thread_data[i].error_count;
    }

    // buscar la url mas visitada
    int max_url_index = find_max_counter_index(final_url_counts, final_url_size);

    // imprimir resultados
    printf("Total Unique IPs: %d\n", final_ip_size);

    if (max_url_index >= 0) {
        printf("Most Visited URL: %s (%d times)\n",
               final_url_counts[max_url_index].key,
               final_url_counts[max_url_index].count);
    }

    printf("HTTP Errors: %d\n", total_errors);

    // liberar memoria
    free_log_lines(lines, line_count);
    free(threads);
    free(thread_data);

    return 0;
}
