#include <SDL2/SDL.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 700

#define TOTAL_CARS 10
#define PARKING_SPOTS 3

#define CAR_W 60
#define CAR_H 30

#define WAIT_LINE_X 140
#define WAIT_LINE_Y 560
#define WAIT_SPACING 85

#define PARK_Y 220
#define PARK_START_X 430
#define PARK_SPACING 180

typedef enum {
    CAR_NOT_ARRIVED,
    CAR_WAITING,
    CAR_MOVING_TO_SPOT,
    CAR_PARKED,
    CAR_MOVING_OUT,
    CAR_FINISHED
} CarState;

typedef struct {
    int id;
    CarState state;

    float x;
    float y;
    float target_x;
    float target_y;

    int assigned_spot;
    int queue_index;

    double wait_time;
    double arrival_time;
    int park_duration;
} Car;

sem_t parking_semaphore;
pthread_mutex_t car_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

Car cars[TOTAL_CARS];
int spot_taken[PARKING_SPOTS] = {0};

int total_cars_parked = 0;
double total_wait_time = 0.0;
int simulation_finished = 0;

double now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int random_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void log_event(int car_id, const char *msg, double waited, int include_wait) {
    pthread_mutex_lock(&log_mutex);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer), "[%a %b %d %H:%M:%S %Y]", tm_info);

    if (include_wait) {
        printf("%s Car %d: %s (waited %.2f seconds)\n", time_buffer, car_id, msg, waited);
    } else {
        printf("%s Car %d: %s\n", time_buffer, car_id, msg);
    }

    fflush(stdout);
    pthread_mutex_unlock(&log_mutex);
}

int get_waiting_position(int car_id) {
    int pos = 0;
    for (int i = 0; i < TOTAL_CARS; i++) {
        if (i == car_id) continue;
        if (cars[i].state == CAR_WAITING && cars[i].arrival_time < cars[car_id].arrival_time) {
            pos++;
        }
    }
    return pos;
}

int assign_free_spot() {
    for (int i = 0; i < PARKING_SPOTS; i++) {
        if (!spot_taken[i]) {
            spot_taken[i] = 1;
            return i;
        }
    }
    return -1;
}

void release_spot(int spot) {
    if (spot >= 0 && spot < PARKING_SPOTS) {
        spot_taken[spot] = 0;
    }
}

void update_waiting_targets() {
    int ordered[TOTAL_CARS];
    int count = 0;

    for (int i = 0; i < TOTAL_CARS; i++) {
        if (cars[i].state == CAR_WAITING) {
            ordered[count++] = i;
        }
    }

    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (cars[ordered[i]].arrival_time > cars[ordered[j]].arrival_time) {
                int tmp = ordered[i];
                ordered[i] = ordered[j];
                ordered[j] = tmp;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        int id = ordered[i];
        cars[id].queue_index = i;
        cars[id].target_x = WAIT_LINE_X + i * WAIT_SPACING;
        cars[id].target_y = WAIT_LINE_Y;
    }
}

void move_car_smoothly(Car *car, float speed) {
    float dx = car->target_x - car->x;
    float dy = car->target_y - car->y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < speed || dist == 0.0f) {
        car->x = car->target_x;
        car->y = car->target_y;
        return;
    }

    car->x += (dx / dist) * speed;
    car->y += (dy / dist) * speed;
}

int car_reached_target(Car *car) {
    return fabsf(car->x - car->target_x) < 2.0f && fabsf(car->y - car->target_y) < 2.0f;
}

void *car_thread(void *arg) {
    int id = *(int *)arg;
    free(arg);

    usleep(random_range(200, 2500) * 1000);

    pthread_mutex_lock(&car_mutex);
    cars[id].state = CAR_WAITING;
    cars[id].arrival_time = now_seconds();
    cars[id].x = -100.0f;
    cars[id].y = WAIT_LINE_Y;
    update_waiting_targets();
    pthread_mutex_unlock(&car_mutex);

    log_event(id, "Arrived at parking lot", 0.0, 0);

    double wait_start = now_seconds();
    sem_wait(&parking_semaphore);
    double wait_end = now_seconds();

    pthread_mutex_lock(&car_mutex);

    cars[id].wait_time = wait_end - wait_start;
    int spot = assign_free_spot();
    cars[id].assigned_spot = spot;
    cars[id].state = CAR_MOVING_TO_SPOT;
    cars[id].target_x = PARK_START_X + spot * PARK_SPACING;
    cars[id].target_y = PARK_Y;

    update_waiting_targets();

    pthread_mutex_unlock(&car_mutex);

    pthread_mutex_lock(&stats_mutex);
    total_cars_parked++;
    total_wait_time += cars[id].wait_time;
    pthread_mutex_unlock(&stats_mutex);

    log_event(id, "Parked successfully", cars[id].wait_time, 1);

    int parked = 0;
    while (!parked) {
        usleep(50000);
        pthread_mutex_lock(&car_mutex);
        if (car_reached_target(&cars[id])) {
            cars[id].state = CAR_PARKED;
            parked = 1;
        }
        pthread_mutex_unlock(&car_mutex);
    }

    cars[id].park_duration = random_range(1, 5);
    sleep(cars[id].park_duration);

    pthread_mutex_lock(&car_mutex);

    int old_spot = cars[id].assigned_spot;
    release_spot(old_spot);

    cars[id].state = CAR_MOVING_OUT;
    cars[id].assigned_spot = -1;
    cars[id].target_x = WINDOW_WIDTH + 120.0f;
    cars[id].target_y = 100.0f + (id % 4) * 70.0f;

    update_waiting_targets();

    pthread_mutex_unlock(&car_mutex);

    sem_post(&parking_semaphore);
    log_event(id, "Leaving parking lot", 0.0, 0);

    int finished = 0;
    while (!finished) {
        usleep(50000);
        pthread_mutex_lock(&car_mutex);
        if (cars[id].x >= WINDOW_WIDTH + 80) {
            cars[id].state = CAR_FINISHED;
            finished = 1;
        }
        pthread_mutex_unlock(&car_mutex);
    }

    return NULL;
}

void draw_parking_lot(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 35, 35, 35, 255);
    SDL_Rect road = {0, 500, WINDOW_WIDTH, 140};
    SDL_RenderFillRect(renderer, &road);

    SDL_SetRenderDrawColor(renderer, 70, 130, 70, 255);
    SDL_Rect grass = {0, 0, WINDOW_WIDTH, 500};
    SDL_RenderFillRect(renderer, &grass);

    for (int i = 0; i < PARKING_SPOTS; i++) {
        int x = PARK_START_X + i * PARK_SPACING - 10;
        SDL_Rect spot = {x, PARK_Y - 20, 110, 70};

        if (spot_taken[i]) {
            SDL_SetRenderDrawColor(renderer, 210, 100, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
        }
        SDL_RenderFillRect(renderer, &spot);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &spot);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    for (int x = 0; x < WINDOW_WIDTH; x += 40) {
        SDL_RenderDrawLine(renderer, x, 570, x + 20, 570);
    }
}

void draw_car(SDL_Renderer *renderer, Car *car) {
    SDL_Rect body = {(int)car->x, (int)car->y, CAR_W, CAR_H};

    switch (car->state) {
        case CAR_WAITING:
            SDL_SetRenderDrawColor(renderer, 255, 193, 7, 255);
            break;
        case CAR_MOVING_TO_SPOT:
            SDL_SetRenderDrawColor(renderer, 52, 152, 219, 255);
            break;
        case CAR_PARKED:
            SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
            break;
        case CAR_MOVING_OUT:
            SDL_SetRenderDrawColor(renderer, 155, 89, 182, 255);
            break;
        case CAR_FINISHED:
            SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
            break;
        default:
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            break;
    }

    SDL_RenderFillRect(renderer, &body);

    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_Rect w1 = {(int)car->x + 8, (int)car->y + CAR_H - 4, 10, 4};
    SDL_Rect w2 = {(int)car->x + CAR_W - 18, (int)car->y + CAR_H - 4, 10, 4};
    SDL_RenderFillRect(renderer, &w1);
    SDL_RenderFillRect(renderer, &w2);

    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderDrawRect(renderer, &body);
}

int main() {
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Smart Parking Lot - SDL2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );

    if (!window) {
        printf("SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    sem_init(&parking_semaphore, 0, PARKING_SPOTS);

    for (int i = 0; i < TOTAL_CARS; i++) {
        cars[i].id = i;
        cars[i].state = CAR_NOT_ARRIVED;
        cars[i].x = -100.0f;
        cars[i].y = WAIT_LINE_Y;
        cars[i].target_x = -100.0f;
        cars[i].target_y = WAIT_LINE_Y;
        cars[i].assigned_spot = -1;
        cars[i].queue_index = -1;
        cars[i].wait_time = 0.0;
        cars[i].arrival_time = 0.0;
        cars[i].park_duration = 0;
    }

    pthread_t threads[TOTAL_CARS];
    for (int i = 0; i < TOTAL_CARS; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, car_thread, id);
    }

    int quit = 0;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

        pthread_mutex_lock(&car_mutex);

        for (int i = 0; i < TOTAL_CARS; i++) {
            if (cars[i].state == CAR_WAITING ||
                cars[i].state == CAR_MOVING_TO_SPOT ||
                cars[i].state == CAR_MOVING_OUT) {
                move_car_smoothly(&cars[i], 2.4f);
            }
        }

        int finished_count = 0;
        for (int i = 0; i < TOTAL_CARS; i++) {
            if (cars[i].state == CAR_FINISHED) {
                finished_count++;
            }
        }
        if (finished_count == TOTAL_CARS) {
            simulation_finished = 1;
        }

        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
        SDL_RenderClear(renderer);

        draw_parking_lot(renderer);

        for (int i = 0; i < TOTAL_CARS; i++) {
            if (cars[i].state != CAR_NOT_ARRIVED && cars[i].state != CAR_FINISHED) {
                draw_car(renderer, &cars[i]);
            }
        }

        pthread_mutex_unlock(&car_mutex);

        SDL_RenderPresent(renderer);

        if (simulation_finished) {
            SDL_Delay(2500);
            quit = 1;
        }

        SDL_Delay(16);
    }

    for (int i = 0; i < TOTAL_CARS; i++) {
        pthread_join(threads[i], NULL);
    }

    double average_wait = 0.0;
    if (total_cars_parked > 0) {
        average_wait = total_wait_time / total_cars_parked;
    }

    printf("Total cars parked: %d\n", total_cars_parked);
    printf("Average wait time: %.2f seconds\n", average_wait);

    sem_destroy(&parking_semaphore);
    pthread_mutex_destroy(&car_mutex);
    pthread_mutex_destroy(&stats_mutex);
    pthread_mutex_destroy(&log_mutex);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
