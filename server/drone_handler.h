#ifndef DRONE_HANDLER_H
#define DRONE_HANDLER_H
#include <pthread.h>
#include "coord.h"

#define MAX_DRONES 100

typedef struct {
    char id[16];
    int x, y;
    int target_x, target_y;
    int busy;
    time_t last_seen;
    int disconnected;
    time_t mission_start_time;
} DroneInfo;

extern DroneInfo drone_list[MAX_DRONES];
extern int drone_count;
extern pthread_mutex_t drone_list_lock;


void* handle_drone(void* arg);

#endif

