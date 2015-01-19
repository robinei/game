#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <cstdint>

#include <vector>
#include <bitset>
#include <algorithm>

#include <SDL2/SDL.h>

using std::vector;
using std::pair;
using std::make_pair;
using std::swap;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

static SDL_Window *window;
static SDL_Renderer *renderer;

#include "math.h"
#include "zorder.cpp"
#include "entity.cpp"




int main(int argc, char *args[]) {
    BitVector bits;
    bits.set(50, true);
    bits.set(125, true);
    assert(bits.is_set(125));
    bits.clear();
    bits.set(51, true);
    assert(!bits.is_set(50));
    assert(!bits.is_set(125));
    assert(bits.is_set(51));

    vector<v2> points {
        v2{6, 6},
        v2{20, 30},
        v2{20, 40}
    };
    
    vector<u32> zindex;
    zorder::make_index(points, zindex);


    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(
        "Game",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1024,
        720,
        0//SDL_WINDOW_ALLOW_HIGHDPI //SDL_WINDOW_OPENGL
    );

    if (!window) {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool dragging = false;
    int orig_x, orig_y;

    v2 p0{4, 4};
    v2 p1{127, 100};

    bool quit = false;
    while (!quit) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        do {
            switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_QUIT:
                quit = true;
                break;
            }
        } while (SDL_PollEvent(&event));


        if (!dragging && (SDL_GetMouseState(&orig_x, &orig_y) & SDL_BUTTON(SDL_BUTTON_LEFT))) {
            if (orig_x > 0 && orig_y > 0)
                dragging = true;
        }
        if (dragging) {
            int x = orig_x;
            int y = orig_y;
            Uint32 state = SDL_GetMouseState(&x, &y);
            if (state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                p0.x = orig_x / zorder::gridDim;
                p0.y = orig_y / zorder::gridDim;
                p1.x = x / zorder::gridDim;
                p1.y = y / zorder::gridDim;
            } else {
                dragging = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        zorder::debug_draw_range(make_pair(zorder::interleave(0, 0),
                                           zorder::interleave(200, 130)));

        vector<v2> result;
        zorder::area_lookup(zindex, p0, p1, result);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
