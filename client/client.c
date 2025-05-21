#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include <fcntl.h> // non-blocking için
#include "protocol.h"

int main(int argc, char* argv[]) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    // === Drone ID ===
    char drone_id[16] = "D1";
    if (argc > 1) {
        strncpy(drone_id, argv[1], sizeof(drone_id) - 1);
    }

    // === Başlangıç Konumu ===
    int x = rand() % 40;
    int y = rand() % 30;
    int target_x = -1, target_y = -1;
    int has_mission = 0;

    // === Socket'i non-blocking moda al (opsiyonel) ===
    fcntl(sock, F_SETFL, O_NONBLOCK);

    while (1) {
        // === STATUS_UPDATE gönder ===
        struct json_object* status = json_object_new_object();
        json_object_object_add(status, MSG_TYPE, json_object_new_string(MSG_STATUS_UPDATE));
        json_object_object_add(status, "drone_id", json_object_new_string(drone_id));

        struct json_object* loc = json_object_new_array();
        json_object_array_add(loc, json_object_new_int(x));
        json_object_array_add(loc, json_object_new_int(y));
        json_object_object_add(status, "location", loc);

        const char* msg = json_object_to_json_string(status);
        send(sock, msg, strlen(msg), 0);
        json_object_put(status);

        // === Cevap bekle (non-blocking) ===
        char buffer[BUFFER_SIZE] = {0};
        int valread = recv(sock, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
        if (valread > 0) {
            buffer[valread] = '\0';

            struct json_object* response = json_tokener_parse(buffer);
            if (response != NULL) {
                const char* type = json_object_get_string(json_object_object_get(response, MSG_TYPE));

                if (strcmp(type, MSG_MISSION) == 0 && !has_mission) {
                    struct json_object* tgt = json_object_object_get(response, "target");
                    target_x = json_object_get_int(json_object_array_get_idx(tgt, 0));
                    target_y = json_object_get_int(json_object_array_get_idx(tgt, 1));
                    has_mission = 1;

                    printf("🛰  %s görev alındı: (%d, %d)\n", drone_id, target_x, target_y);
                }
                json_object_put(response);
            }
        }

        // === Görev varsa hedefe ilerle ===
        if (has_mission) {
            if (x < target_x) x++;
            else if (x > target_x) x--;

            if (y < target_y) y++;
            else if (y > target_y) y--;

            printf("📍 %s → (%d, %d) → hedef: (%d, %d)\n", drone_id, x, y, target_x, target_y);

            // Hedefe ulaşıldıysa:
            if (x == target_x && y == target_y) {
                sleep(1); // Görev süresi 0 çıkmasın

                struct json_object* complete = json_object_new_object();
                json_object_object_add(complete, MSG_TYPE, json_object_new_string(MSG_MISSION_COMPLETE));
                json_object_object_add(complete, "drone_id", json_object_new_string(drone_id));
                const char* complete_msg = json_object_to_json_string(complete);
                send(sock, complete_msg, strlen(complete_msg), 0);
                json_object_put(complete);

                printf("✅ %s görev tamamlandı!\n", drone_id);
                has_mission = 0;
                target_x = target_y = -1;
            }
        }

        sleep(1);
    }

    close(sock);
    return 0;
}

