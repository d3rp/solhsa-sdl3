#include <string>
#include <cmath>
#include <cstdlib>

#include "stopwatch.hpp"

#ifdef __EMSCRIPTEN__
#    include <emscripten/emscripten.h>
#endif

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

struct SdlGlobal
{
    bool done { false };
    unsigned int* framebuffer { nullptr };
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
void init_snow()
{
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
        g.framebuffer[i] = 0xff000000;

    for (int i = 0; i < WINDOW_WIDTH; i++)
    {
        int p = (int)((sin((i + 3247) * 0.02) * 0.3 +
                        sin((i + 2347) * 0.04) * 0.1 +
                        sin((i + 4378) * 0.01) * 0.6) * 100 + (WINDOW_HEIGHT * 2 / 3));
        int pos = p * WINDOW_WIDTH + i;
        for (int j = p; j < WINDOW_HEIGHT; j++)
        {
            g.framebuffer[pos] = 0xff007f00;
            pos += WINDOW_WIDTH;
        }
    }
}

void newsnow()
{
    for (int i = 0; i < 8; i++)
        g.framebuffer[rand() % (WINDOW_WIDTH - 2) + 1] = 0xffffffff;
}

void snowfall()
{
    STOPWATCH("snowfall");
    for (int j = WINDOW_HEIGHT - 2; j >= 0; j--)
    {
        int ypos = j * WINDOW_WIDTH;
        for (int i = 1; i < WINDOW_WIDTH - 1; i++)
        {
            constexpr int white_pxl = (int) 0xffffffff;
            if (g.framebuffer[ypos + i] < white_pxl)
                continue;

            if (g.framebuffer[ypos + i + WINDOW_WIDTH] == 0xff000000)
            {
                g.framebuffer[ypos + i + WINDOW_WIDTH] = 0xffffffff;
                g.framebuffer[ypos + i] = 0xff000000;
            }
            else
                if (g.framebuffer[ypos + i + WINDOW_WIDTH - 1] == 0xff000000)
            {
                g.framebuffer[ypos + i + WINDOW_WIDTH - 1] = 0xffffffff;
                g.framebuffer[ypos + i] = 0xff000000;
            }
            else
                if (g.framebuffer[ypos + i + WINDOW_WIDTH + 1] == 0xff000000)
            {
                g.framebuffer[ypos + i + WINDOW_WIDTH + 1] = 0xffffffff;
                g.framebuffer[ypos + i] = 0xff000000;
            }
        }
    }
}
// clang-format off
const unsigned char sprite[] =
    {
        0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
        0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
        0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,
        0,1,1,1,1,0,0,0,0,0,0,1,1,1,1,0,
        0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,
        0,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,
        1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,
        1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,
        1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,
        1,1,0,0,0,1,1,1,1,1,1,0,0,0,1,1,
        0,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,
        0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,0,
        0,1,1,1,1,0,0,0,0,0,0,1,1,1,1,0,
        0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,
        0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
        0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0
    };
// clang-format on

void drawsprite(int x, int y, unsigned int color)
{
    int i, j, c, yofs;
    yofs = y * WINDOW_WIDTH + x;
    for (i = 0, c = 0; i < 16; i++)
    {
        for (j = 0; j < 16; j++, c++)
            if (sprite[c])
                g.framebuffer[yofs + j] = color;

        yofs += WINDOW_WIDTH;
    }
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
    newsnow();
    snowfall();
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

    g.framebuffer = new unsigned int[WINDOW_WIDTH * WINDOW_HEIGHT];
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

    init_snow();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (!g.done)
        loop();
#endif

    return 0;
}
