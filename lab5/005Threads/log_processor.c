#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log_processor.h"

// leer archivo completo y guardar cada linea
char **read_log_file(const char *filename, int *line_count) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    // capacidad inicial
    int capacity = 1024;

    // arreglo de lineas
    char **lines = malloc(sizeof(char *) * capacity);
    if (lines == NULL) {
        fclose(file);
        return NULL;
    }

    *line_count = 0;
    char buffer[MAX_LINE_LENGTH];

    // leer linea por linea
    while (fgets(buffer, sizeof(buffer), file) != NULL) {

        // si se llena el arreglo, duplicar tamaño
        if (*line_count >= capacity) {
            capacity *= 2;
            char **temp = realloc(lines, sizeof(char *) * capacity);
            if (temp == NULL) {
                free_log_lines(lines, *line_count);
                fclose(file);
                return NULL;
            }
            lines = temp;
        }

        // reservar memoria para la linea
        lines[*line_count] = malloc(strlen(buffer) + 1);
        if (lines[*line_count] == NULL) {
            free_log_lines(lines, *line_count);
            fclose(file);
            return NULL;
        }

        // copiar linea al arreglo
        strcpy(lines[*line_count], buffer);

        (*line_count)++;
    }

    fclose(file);
    return lines;
}

// liberar memoria de las lineas
void free_log_lines(char **lines, int line_count) {

    if (lines == NULL) {
        return;
    }

    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }

    free(lines);
}

// extraer ip, url y status de una linea del log
int parse_log_line(const char *line, char *ip, char *url, int *status) {

    char method[32];

    int result = sscanf(
        line,
        "%127s - - [%*[^]]] \"%31s %127[^\"]\" %d",
        ip,
        method,
        url,
        status
    );

    // si pudo leer ip, metodo, url y status
    return (result == 4);
}

// sumar contador de una clave
void increment_counter(Counter *arr, int *size, const char *key) {

    // buscar si ya existe
    for (int i = 0; i < *size; i++) {

        if (strcmp(arr[i].key, key) == 0) {
            arr[i].count++;
            return;
        }
    }

    // si no existe, agregar nueva entrada
    if (*size < MAX_COUNTERS) {

        strcpy(arr[*size].key, key);
        arr[*size].count = 1;
        (*size)++;
    }
}

// combinar contadores de un thread con el arreglo final
void merge_counters(Counter *dest, int *dest_size, Counter *src, int src_size) {

    for (int i = 0; i < src_size; i++) {

        int found = 0;

        // buscar si ya existe en destino
        for (int j = 0; j < *dest_size; j++) {

            if (strcmp(dest[j].key, src[i].key) == 0) {

                dest[j].count += src[i].count;
                found = 1;
                break;
            }
        }

        // si no existe, agregar
        if (!found && *dest_size < MAX_COUNTERS) {

            strcpy(dest[*dest_size].key, src[i].key);
            dest[*dest_size].count = src[i].count;
            (*dest_size)++;
        }
    }
}

// buscar indice del contador mas grande
int find_max_counter_index(Counter *arr, int size) {

    if (size <= 0) {
        return -1;
    }

    int max_index = 0;

    for (int i = 1; i < size; i++) {

        if (arr[i].count > arr[max_index].count) {
            max_index = i;
        }
    }

    return max_index;
}

// funcion que ejecuta cada thread
void *process_chunk(void *arg) {

    ThreadData *data = (ThreadData *)arg;

    char ip[MAX_KEY_LENGTH];
    char url[MAX_KEY_LENGTH];
    int status;

    // recorrer lineas asignadas al thread
    for (int i = data->start; i < data->end; i++) {

        // parsear linea
        if (parse_log_line(data->lines[i], ip, url, &status)) {

            // contar ip
            increment_counter(data->ip_counts, &data->ip_size, ip);

            // contar url
            increment_counter(data->url_counts, &data->url_size, url);

            // contar errores http
            if (status >= 400 && status <= 599) {
                data->error_count++;
            }
        }
    }

    return NULL;
}
