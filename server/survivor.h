#ifndef SURVIVOR_H
#define SURVIVOR_H

#include "coord.h"
#include "list.h"
#include <pthread.h>

typedef struct {
    int id;
    Coord coord;
    int helped; // 0 = yardım bekliyor, 1 = yardım edildi
} Survivor;

// ✅ TANIMLAR BURADA SADECE "extern" OLARAK
extern List* survivors;
extern pthread_mutex_t survivor_list_lock;

void* survivor_generator(void* arg);

#endif

