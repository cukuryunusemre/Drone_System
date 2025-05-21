#ifndef VIEW_H
#define VIEW_H

#include "list.h"
#include <SDL2/SDL.h>

int init_sdl_window(int grid_width, int grid_height);
void draw_frame(List* survivor_snapshot, List* drone_snapshot);
void close_sdl_window();
void* view_loop(void* arg);

#endif

