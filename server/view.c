#include "view.h"
#include "survivor.h"
#include "coord.h"
#include <SDL2/SDL.h>
#include "drone_handler.h"
#include <unistd.h>

#define CELL_SIZE 20

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int init_sdl_window(int grid_width, int grid_height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    window = SDL_CreateWindow("Drone Visualizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        grid_width * CELL_SIZE, grid_height * CELL_SIZE, 0);

    if (!window) return 1;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 1;

    return 0;
}

void draw_frame(List* survivor_snapshot, List* drone_snapshot) {
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); // background
    SDL_RenderClear(renderer);

    // Survivor: red
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    Node* s = survivor_snapshot->head;
    while (s) {
        Survivor* sv = (Survivor*)s->data;
        SDL_Rect rect = { sv->coord.x * CELL_SIZE, sv->coord.y * CELL_SIZE, CELL_SIZE, CELL_SIZE };
        SDL_RenderFillRect(renderer, &rect);
        s = s->next;
    }

    // Drone: blue circle
    SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
    Node* d = drone_snapshot->head;
    while (d) {
        Coord* coord = (Coord*)d->data;
        SDL_Rect drone_rect = { coord->x * CELL_SIZE + CELL_SIZE / 4, coord->y * CELL_SIZE + CELL_SIZE / 4, CELL_SIZE / 2, CELL_SIZE / 2 };
        SDL_RenderFillRect(renderer, &drone_rect);
        d = d->next;
    }

    SDL_RenderPresent(renderer);
}

void close_sdl_window() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


List* snapshot_drone_coords() {
    List* snapshot = create_list(sizeof(Coord));

    pthread_mutex_lock(&drone_list_lock);
    for (int i = 0; i < drone_count; i++) {
        if (drone_list[i].disconnected == 0) {
            Coord c = { drone_list[i].x, drone_list[i].y };
            add_to_list(snapshot, &c);
        }
    }
    pthread_mutex_unlock(&drone_list_lock);

    return snapshot;
}

List* snapshot_survivors() {
    List* snapshot = create_list(sizeof(Survivor));

    pthread_mutex_lock(&survivor_list_lock);
    Node* node = survivors->head;
    while (node) {
        Survivor* s = (Survivor*)node->data;
        if (s->helped == 0) {
            add_to_list(snapshot, s);
        }
        node = node->next;
    }
    pthread_mutex_unlock(&survivor_list_lock);

    return snapshot;
}

void* view_loop(void* arg) {
    init_sdl_window(40, 30); // Grid boyutu sabit

    while (1) {
        List* drone_coords = snapshot_drone_coords();
        List* survivor_copy = snapshot_survivors();

        draw_frame(survivor_copy, drone_coords);

        destroy_list(drone_coords);
        destroy_list(survivor_copy);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                close_sdl_window();
                exit(0);
            }
        }

        sleep(1);
    }

    return NULL;
}
