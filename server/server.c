#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "protocol.h"
#include "drone_handler.h"
#include "survivor.h"
#include "list.h"
#include "view.h"


extern void* timeout_checker(void*); // timeout_checker drone_handler.c'de

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        return 1;
    }

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    printf("📡 Server listening on port %d...\n", PORT);
    
    pthread_t view_thread;
pthread_create(&view_thread, NULL, view_loop, NULL);


    // ✅ Burada SADECE create_list çağrısı var
    survivors = create_list(sizeof(Survivor));

    pthread_t survivor_thread;
    pthread_create(&survivor_thread, NULL, survivor_generator, NULL);

    pthread_t timeout_thread;
    pthread_create(&timeout_thread, NULL, timeout_checker, NULL);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t tid;
        int* arg = malloc(sizeof(int));
        *arg = client_fd;
        pthread_create(&tid, NULL, handle_drone, arg);
    }

    close(server_fd);
    return 0;
}

