#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_STUDENTS 10
#define BRIDGE_CAPACITY 4
#define RIGHT 0
#define LEFT  1
#define NONE -1

typedef struct {
    int id;
    int direction;
    struct timespec arrival_time;
} Student;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    int on_bridge;              // cuántos van actualmente sobre el puente
    int current_direction;      // dirección actual del puente: LEFT, RIGHT o NONE

    int waiting_left;
    int waiting_right;

    int consecutive_same_dir;   // cuántos han pasado seguidos en esta dirección
    int max_consecutive;        // límite para favorecer cambio y evitar starvation

    double total_waiting_time;
    int crossed_students;
} Bridge;

Bridge bridge;

const char *direction_to_string(int dir) {
    return (dir == LEFT) ? "Left" : "Right";
}

double diff_seconds(struct timespec start, struct timespec end) {
    double sec = (double)(end.tv_sec - start.tv_sec);
    double nsec = (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
    return sec + nsec;
}

void print_status_locked(const char *action, int student_id, int direction) {
    printf(
        "Inge %02d %s %s | bridge=%d dir=%s | waiting(L=%d,R=%d)\n",
        student_id,
        action,
        (action[0] == 'a') ? direction_to_string(direction) : "",
        bridge.on_bridge,
        (bridge.current_direction == NONE) ? "None" : direction_to_string(bridge.current_direction),
        bridge.waiting_left,
        bridge.waiting_right
    );
}

void bridge_init(Bridge *b) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);

    b->on_bridge = 0;
    b->current_direction = NONE;
    b->waiting_left = 0;
    b->waiting_right = 0;

    b->consecutive_same_dir = 0;
    b->max_consecutive = 8; // puedes ajustar este valor

    b->total_waiting_time = 0.0;
    b->crossed_students = 0;
}

void bridge_destroy(Bridge *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

int opposite_direction(int dir) {
    return (dir == LEFT) ? RIGHT : LEFT;
}

int waiting_in_direction(int dir) {
    return (dir == LEFT) ? bridge.waiting_left : bridge.waiting_right;
}

int waiting_opposite(int dir) {
    return (dir == LEFT) ? bridge.waiting_right : bridge.waiting_left;
}

/*
Reglas para entrar:
1. Si el puente está vacío, se puede escoger dirección.
2. Si el puente tiene gente, solo puede entrar alguien de la misma dirección.
3. Nunca más de 4 en el puente.
4. Si ya han pasado demasiados seguidos en una dirección y hay gente esperando del otro lado,
   forzamos cambio cuando el puente quede vacío.
*/
void accessBridge(int direction, int student_id, struct timespec arrival_time) {
    pthread_mutex_lock(&bridge.mutex);

    if (direction == LEFT) {
        bridge.waiting_left++;
    } else {
        bridge.waiting_right++;
    }

    printf("Inge %02d arrives wanting to go %s | waiting(L=%d,R=%d)\n",
           student_id, direction_to_string(direction),
           bridge.waiting_left, bridge.waiting_right);

    while (1) {
        int can_enter = 0;

        // Caso 1: puente vacío
        if (bridge.on_bridge == 0) {
            int opposite_waiting = waiting_opposite(direction);

            // Si venimos de una racha larga y hay del otro lado esperando,
            // no dejamos seguir a esta dirección todavía.
            if (!(bridge.current_direction == direction &&
                  bridge.consecutive_same_dir >= bridge.max_consecutive &&
                  opposite_waiting > 0)) {
                can_enter = 1;
            }
        }
        // Caso 2: puente ocupado
        else if (bridge.current_direction == direction &&
                 bridge.on_bridge < BRIDGE_CAPACITY) {
            can_enter = 1;
        }

        if (can_enter) {
            break;
        }

        pthread_cond_wait(&bridge.cond, &bridge.mutex);
    }

    if (direction == LEFT) {
        bridge.waiting_left--;
    } else {
        bridge.waiting_right--;
    }

    // Si estaba vacío, fijamos la dirección
    if (bridge.on_bridge == 0) {
        if (bridge.current_direction != direction) {
            bridge.consecutive_same_dir = 0;
        }
        bridge.current_direction = direction;
    }

    bridge.on_bridge++;
    bridge.consecutive_same_dir++;

    struct timespec enter_time;
    clock_gettime(CLOCK_MONOTONIC, &enter_time);
    double wait_time = diff_seconds(arrival_time, enter_time);

    bridge.total_waiting_time += wait_time;
    bridge.crossed_students++;

    printf("Inge %02d crosses to the %s (on bridge: %d) | waited %.3f s | waiting(L=%d,R=%d)\n",
           student_id,
           direction_to_string(direction),
           bridge.on_bridge,
           wait_time,
           bridge.waiting_left,
           bridge.waiting_right);

    pthread_mutex_unlock(&bridge.mutex);
}

void exitBridge(int direction, int student_id) {
    pthread_mutex_lock(&bridge.mutex);

    bridge.on_bridge--;

    printf("Inge %02d exits bridge (on bridge: %d) | dir=%s | waiting(L=%d,R=%d)\n",
           student_id,
           bridge.on_bridge,
           direction_to_string(direction),
           bridge.waiting_left,
           bridge.waiting_right);

    // Si quedó vacío, evaluamos si conviene cambiar de dirección
    if (bridge.on_bridge == 0) {
        int opposite = opposite_direction(direction);
        int opposite_wait = waiting_in_direction(opposite);
        int same_wait = waiting_in_direction(direction);

        if (opposite_wait > 0 &&
            (bridge.consecutive_same_dir >= bridge.max_consecutive || same_wait == 0)) {
            bridge.current_direction = opposite;
            bridge.consecutive_same_dir = 0;
        } else if (same_wait == 0 && opposite_wait == 0) {
            bridge.current_direction = NONE;
            bridge.consecutive_same_dir = 0;
        }
        // si no, se mantiene la dirección actual para el siguiente grupo
    }

    pthread_cond_broadcast(&bridge.cond);
    pthread_mutex_unlock(&bridge.mutex);
}

void *student_thread(void *arg) {
    Student *student = (Student *)arg;

    // Espera aleatoria antes de intentar cruzar (0 a 5 segundos)
    int arrival_delay = rand() % 6;
    sleep(arrival_delay);

    clock_gettime(CLOCK_MONOTONIC, &student->arrival_time);

    accessBridge(student->direction, student->id, student->arrival_time);

    // Tiempo de cruce entre 1 y 3 segundos
    int crossing_time = (rand() % 3) + 1;
    sleep(crossing_time);

    exitBridge(student->direction, student->id);

    return NULL;
}

int main() {
    srand((unsigned int)time(NULL));

    pthread_t threads[NUM_STUDENTS];
    Student students[NUM_STUDENTS];

    bridge_init(&bridge);

    for (int i = 0; i < NUM_STUDENTS; i++) {
        students[i].id = i + 1;
        students[i].direction = rand() % 2; // 0 = Right, 1 = Left

        if (pthread_create(&threads[i], NULL, student_thread, &students[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n===== FINAL REPORT =====\n");
    printf("Total students crossed: %d\n", bridge.crossed_students);
    if (bridge.crossed_students > 0) {
        printf("Average waiting time: %.3f seconds\n",
               bridge.total_waiting_time / bridge.crossed_students);
    } else {
        printf("Average waiting time: 0.000 seconds\n");
    }

    bridge_destroy(&bridge);
    return 0;
}
