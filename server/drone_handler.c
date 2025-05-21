#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#include "protocol.h"
#include "coord.h"
#include "list.h"
#include "survivor.h"

// === Performans Takibi ===
int mission_count = 0;
int disconnect_count = 0;
double total_mission_time = 0.0;


// === Drone Bilgisi ===
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

DroneInfo drone_list[MAX_DRONES];
int drone_count = 0;
pthread_mutex_t drone_list_lock = PTHREAD_MUTEX_INITIALIZER;

// === En Yakın Yardım Edilmemiş Survivor'ı Bul ===
Survivor* find_closest_unhelped_survivor(int x, int y) {
    pthread_mutex_lock(&survivor_list_lock);

    Survivor* closest = NULL;
    int min_dist = 1e9;
    Node* current = survivors->head;

    while (current) {
        Survivor* s = (Survivor*)current->data;
        if (s->helped == 0) {
            int dist = abs(s->coord.x - x) + abs(s->coord.y - y);
            if (dist < min_dist) {
                closest = s;
                min_dist = dist;
            }
        }
        current = current->next;
    }

    if (closest != NULL) {
        closest->helped = 1; // ✅ ATANDIĞI ANDA AYNI ANDA KİMSE ALAMASIN
    }

    pthread_mutex_unlock(&survivor_list_lock);
    return closest;
}


// === Drone Bilgisini Güncelle veya Yeni Drone Ekle ===
void update_drone_list(const char* id, int x, int y) {
    pthread_mutex_lock(&drone_list_lock);

    for (int i = 0; i < drone_count; i++) {
        if (strcmp(drone_list[i].id, id) == 0) {
            drone_list[i].x = x;
            drone_list[i].y = y;
            drone_list[i].last_seen = time(NULL);
            drone_list[i].disconnected = 0;
            pthread_mutex_unlock(&drone_list_lock);
            return;
        }
    }

    if (drone_count < MAX_DRONES) {
        strncpy(drone_list[drone_count].id, id, sizeof(drone_list[drone_count].id));
        drone_list[drone_count].x = x;
        drone_list[drone_count].y = y;
        drone_list[drone_count].busy = 0;
        drone_list[drone_count].last_seen = time(NULL);
        drone_list[drone_count].disconnected = 0;
        drone_count++;
    }

    pthread_mutex_unlock(&drone_list_lock);
}

// === Görev Zamanı Geldiğinde Çalışan Drone Handler ===
void* handle_drone(void* arg) {
    int client_fd = *((int*)arg);
    free(arg);

    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) break;
        buffer[valread] = '\0';

        struct json_object* parsed = json_tokener_parse(buffer);
        const char* msg_type = json_object_get_string(json_object_object_get(parsed, MSG_TYPE));

        // === STATUS_UPDATE ===
        if (strcmp(msg_type, MSG_STATUS_UPDATE) == 0) {
            const char* drone_id = json_object_get_string(json_object_object_get(parsed, "drone_id"));
            struct json_object* loc = json_object_object_get(parsed, "location");
            int x = json_object_get_int(json_object_array_get_idx(loc, 0));
            int y = json_object_get_int(json_object_array_get_idx(loc, 1));

            update_drone_list(drone_id, x, y);

            DroneInfo* drone = NULL;
            pthread_mutex_lock(&drone_list_lock);
            for (int i = 0; i < drone_count; i++) {
                if (strcmp(drone_list[i].id, drone_id) == 0) {
                    drone = &drone_list[i];
                    break;
                }
            }

            if (drone && drone->busy == 0 && drone->disconnected == 0) {
                Survivor* target = find_closest_unhelped_survivor(x, y);

                if (target != NULL) {
                    drone->busy = 1;
                    drone->target_x = target->coord.x;
                    drone->target_y = target->coord.y;
                    
                     drone->mission_start_time = time(NULL);
                     
                     mission_count++;

                    pthread_mutex_lock(&survivor_list_lock);
                    target->helped = 1;
                    pthread_mutex_unlock(&survivor_list_lock);

                    struct json_object* response = json_object_new_object();
                    json_object_object_add(response, MSG_TYPE, json_object_new_string(MSG_MISSION));

                    struct json_object* tgt = json_object_new_array();
                    json_object_array_add(tgt, json_object_new_int(drone->target_x));
                    json_object_array_add(tgt, json_object_new_int(drone->target_y));
                    json_object_object_add(response, "target", tgt);

                    const char* resp_str = json_object_to_json_string(response);
                    send(client_fd, resp_str, strlen(resp_str), 0);
                    json_object_put(response);

                    printf("🚁 Drone %s → Survivor (%d, %d) görevlendirildi.\n", drone_id, target->coord.x, target->coord.y);
                }
            }

            pthread_mutex_unlock(&drone_list_lock);
        }

        // === MISSION_COMPLETE ===
        else if (strcmp(msg_type, MSG_MISSION_COMPLETE) == 0) {
    const char* drone_id = json_object_get_string(json_object_object_get(parsed, "drone_id"));

    pthread_mutex_lock(&drone_list_lock);
    for (int i = 0; i < drone_count; i++) {
        if (strcmp(drone_list[i].id, drone_id) == 0) {
            drone_list[i].busy = 0;

            time_t end = time(NULL);
            double duration = difftime(end, drone_list[i].mission_start_time);
            total_mission_time += duration;

            // ✅ EKLENECEK SATIR
            printf("✅ Drone %s görevi tamamladı. Süre: %.0f sn\n", drone_id, duration);

            break;
        }
    }
    pthread_mutex_unlock(&drone_list_lock);
}

        json_object_put(parsed);
    }

    close(client_fd);
    printf("🔌 Drone bağlantısı kapandı.\n");
    return NULL;
}

// === Timeout Kontrol Thread'i ===
void* timeout_checker(void* arg) {
    while (1) {
        pthread_mutex_lock(&drone_list_lock);
        time_t now = time(NULL);

        for (int i = 0; i < drone_count; i++) {
            if (drone_list[i].disconnected == 0 && (now - drone_list[i].last_seen) > 10) {
                printf("⛔ Drone %s bağlantısı kesildi! (%lds)\n", drone_list[i].id, now - drone_list[i].last_seen);
                drone_list[i].disconnected = 1;
                    disconnect_count++;

                // Görevdeyse survivor tekrar açık hale gelsin
                if (drone_list[i].busy) {
                    int tx = drone_list[i].target_x;
                    int ty = drone_list[i].target_y;

                    pthread_mutex_lock(&survivor_list_lock);
                    Node* cur = survivors->head;
                    while (cur) {
                        Survivor* s = (Survivor*)cur->data;
                        if (s->coord.x == tx && s->coord.y == ty) {
                            s->helped = 0;
                            printf("🔁 Survivor (%d, %d) yeniden atanabilir hale getirildi.\n", tx, ty);
                            break;
                        }
                        cur = cur->next;
                    }
                    pthread_mutex_unlock(&survivor_list_lock);

                    drone_list[i].busy = 0;
                }
            }
        }

        pthread_mutex_unlock(&drone_list_lock);
        sleep(1);
        if (mission_count > 0) {
    double avg_time = total_mission_time / mission_count;
    printf("📊 Görevler: %d | Disconnect: %d | Ortalama Süre: %.1f sn\n",
           mission_count, disconnect_count, avg_time);
}

    }

    return NULL;
}

