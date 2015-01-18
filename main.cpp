#include <vector>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
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




int main() {
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

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    vector<u32> zvals;
    for (u32 y = 0; y < 100; ++y) {
        for (u32 x = 0; x < 100; ++x) {
            u32 z = zorder::interleave(x, y);
            zvals.push_back(z);
        }
    }
    std::sort(zvals.begin(), zvals.end());
    for (u32 i = 0; i + 1 < zvals.size(); ++i) {
        u32 x0 = zorder::deinterleave_x(zvals[i]);
        u32 y0 = zorder::deinterleave_y(zvals[i]);
        u32 x1 = zorder::deinterleave_x(zvals[i + 1]);
        u32 y1 = zorder::deinterleave_y(zvals[i + 1]);
        SDL_RenderDrawLine(renderer, x0*10, y0*10, x1*10, y1*10);
    }

    vector<v2> result;
    zorder::area_lookup(zindex, v2{5, 5}, v2{9, 8}, result);

    SDL_RenderPresent(renderer);

    bool quit = false;
    while (!quit) {
        SDL_Event event;
        SDL_WaitEvent(&event);

        switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_QUIT:
            quit = true;
            break;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
