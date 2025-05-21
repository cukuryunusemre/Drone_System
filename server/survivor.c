#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "survivor.h"
#include "list.h"

// ✅ SADECE BURADA GERÇEK TANIM
List* survivors = NULL;
pthread_mutex_t survivor_list_lock = PTHREAD_MUTEX_INITIALIZER;

int survivor_counter = 0;

void* survivor_generator(void* arg) {
    srand(time(NULL));

    while (1) {
        Survivor s;
        s.id = survivor_counter++;
        s.coord.x = rand() % 40;
        s.coord.y = rand() % 30;
        s.helped = 0;

        printf("🆘 Survivor oluşturuldu → ID: %d (%d, %d)\n", s.id, s.coord.x, s.coord.y);

        pthread_mutex_lock(&survivor_list_lock);
        add_to_list(survivors, &s);
        pthread_mutex_unlock(&survivor_list_lock);

        sleep(2);
    }

    return NULL;
}

