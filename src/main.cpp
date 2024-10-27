#include <string>
#include <cmath>
#include <cstdlib>

#ifdef __EMSCRIPTEN__
#    include <emscripten/emscripten.h>
#endif

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

struct SdlGlobal
{
    bool done { false };
    int* framebuffer { nullptr };
    SDL_Window* window { nullptr };
    SDL_Renderer* renderer { nullptr };
    SDL_Texture* texture { nullptr };
} g;

constexpr int WINDOW_WIDTH = 1920 / 2;
constexpr int WINDOW_HEIGHT = 1080 / 2;

void putpixel(int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= WINDOW_WIDTH || y >= WINDOW_HEIGHT)
    {
        return;
    }
    g.framebuffer[y * WINDOW_WIDTH + x] = color;
}
bool update()
{
    SDL_Event e;
    if (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
            return false;
        if (e.type == SDL_EVENT_KEY_UP && (e.key.key == SDLK_ESCAPE || e.key.key == SDLK_Q))
            return false;
    }

    char* pix;
    int pitch;

    SDL_LockTexture(g.texture, nullptr, (void**) &pix, &pitch);
    for (int i = 0, sp = 0, dp = 0; i < WINDOW_HEIGHT; i++, dp += WINDOW_WIDTH, sp += pitch)
        memcpy(pix + sp, g.framebuffer + dp, WINDOW_WIDTH * 4);

    SDL_UnlockTexture(g.texture);
    SDL_RenderTexture(g.renderer, g.texture, nullptr, nullptr);
    SDL_RenderPresent(g.renderer);
    SDL_Delay(1);
    return true;
}

void render(Uint64 aTicks)
{
    for (int i = 0, c = 0; i < WINDOW_HEIGHT; i++)
    {
        for (int j = 0; j < WINDOW_WIDTH; j++, c++)
        {
            g.framebuffer[c] = (int) (sin(c) + (i * j) - (j * j) + aTicks) / (i + 1) | 0xff403020;
        }
    }
    auto ticks_sin = sin(aTicks / 100);
    auto ticks_cos = cos(aTicks / 50);
    auto ticks_mod = (int) ((aTicks/100) % 10);
    const int w = 30;
    for (int i = 0; i < w; i++)
        for (int j = 0; j < w; j++)
        {
            const auto z =  w/2;
            const auto x = i - z;
            const auto y = j - z;
            if (x*x + y*y > z*z)
                continue;
            putpixel(WINDOW_WIDTH / 2 + ticks_sin * WINDOW_WIDTH / (13 - ticks_mod) + j,
                     WINDOW_HEIGHT / 2 + ticks_cos * WINDOW_HEIGHT / (12 - ticks_mod) + i,
                     ticks_mod*0x00100010 | 0xff000000);
        }
}

void loop()
{
    if (!update())
    {
        g.done = true;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
    }
    else
    {
        render(SDL_GetTicks());
    }
}

bool init()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return false;

    g.framebuffer = new int[WINDOW_WIDTH * WINDOW_HEIGHT];
    g.window = SDL_CreateWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    g.renderer = SDL_CreateRenderer(g.window, nullptr);
    g.texture = SDL_CreateTexture(g.renderer,
                                  SDL_PIXELFORMAT_ABGR8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  WINDOW_WIDTH,
                                  WINDOW_HEIGHT);

    if (!g.framebuffer || !g.window || !g.renderer || !g.texture)
        return false;

    return true;
}

void destroy()
{
    SDL_DestroyTexture(g.texture);
    SDL_DestroyRenderer(g.renderer);
    SDL_DestroyWindow(g.window);
    SDL_Quit();
}

int main(int argc, char** argv)
{
    if (!init())
        return -1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (!g.done)
        loop();
#endif

    return 0;
}
