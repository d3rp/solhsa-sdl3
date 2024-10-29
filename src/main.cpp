#include <string>
#include <cmath>
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

#include "stopwatch.hpp"

#ifdef __EMSCRIPTEN__
#    include <emscripten/emscripten.h>
#endif

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

#define TREECOUNT 32
float gTreeCoord[TREECOUNT * 2];
// Look-up table
unsigned short* gLut;
// Texture
int* gTexture;
// Distance mask
unsigned int* gMask;
struct SdlGlobal
{
    bool done { false };
    int* framebuffer { nullptr };
    unsigned int* tmp_buffer;
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
void init_gfx()
{
    int x, y, n;
    gTexture = (int*) stbi_load("../resources/tunneltexture.png", &x, &y, &n, 4);
    gLut = new unsigned short[WINDOW_WIDTH * WINDOW_HEIGHT * 4];
    gMask = new unsigned int[WINDOW_WIDTH * WINDOW_HEIGHT * 4];

    int i, j;
    for (i = 0; i < WINDOW_HEIGHT * 2; i++)
    {
        for (j = 0; j < WINDOW_WIDTH * 2; j++)
        {
            int xdist = j - (WINDOW_WIDTH);
            int ydist = i - (WINDOW_HEIGHT);

#define DistanceShape 1
#define DistanceFlower false

#if DistanceShape == 1  // circle
            int distance = (int) sqrt((float) (xdist * xdist + ydist * ydist));
#elif DistanceShape == 2  // square
            int distance = (abs(xdist) > abs(ydist)) ? abs(xdist) : abs(ydist);
#elif DistanceShape == 3  // diamond
            int distance = (abs(xdist) + abs(ydist)) / 2;
#endif

#if DistanceFlower
            distance += (int) (sin(atan2((float) xdist, (float) ydist) * 5) * 8);
#endif

            if (distance > 0)
                distance = (64 * 256 / distance) & 0xff;

            int d = distance;
            if (d > 255)
                d = 255;
            gMask[i * WINDOW_WIDTH * 2 + j] = d * 0x010101;

            int angle = (int) (((atan2((float) xdist, (float) ydist) / M_PI) + 1.0f) * 128);

            gLut[i * WINDOW_WIDTH * 2 + j] = (distance << 8) + angle;
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
            else if (g.framebuffer[ypos + i + WINDOW_WIDTH - 1] == 0xff000000)
            {
                g.framebuffer[ypos + i + WINDOW_WIDTH - 1] = 0xffffffff;
                g.framebuffer[ypos + i] = 0xff000000;
            }
            else if (g.framebuffer[ypos + i + WINDOW_WIDTH + 1] == 0xff000000)
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
        0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,
        0,0,0,1,0,1,0,1,0,1,0,1,0,0,0,0,
        0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,
        0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,
        0,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,
        0,1,0,0,0,0,1,1,1,1,0,0,0,1,0,0,
        1,0,0,0,0,1,1,1,1,1,1,0,0,0,1,0,
        0,1,0,0,0,1,1,1,1,1,1,0,0,0,0,1,
        1,0,0,0,0,1,1,1,1,1,1,0,0,0,1,0,
        0,1,0,0,0,1,1,1,1,1,1,0,0,0,0,1,
        0,0,1,0,0,0,1,1,1,1,0,0,0,0,1,0,
        0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,
        0,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,
        0,0,0,1,0,1,0,0,0,1,0,0,1,1,0,0,
        0,0,0,0,1,0,1,0,1,0,1,0,1,0,0,0,
        0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0
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

unsigned int blend_avg(int source, int target)
{
    unsigned int sourcer = ((unsigned int) source >> 0) & 0xff;
    unsigned int sourceg = ((unsigned int) source >> 8) & 0xff;
    unsigned int sourceb = ((unsigned int) source >> 16) & 0xff;
    unsigned int targetr = ((unsigned int) target >> 0) & 0xff;
    unsigned int targetg = ((unsigned int) target >> 8) & 0xff;
    unsigned int targetb = ((unsigned int) target >> 16) & 0xff;

    targetr = (sourcer + targetr) / 2;
    targetg = (sourceg + targetg) / 2;
    targetb = (sourceb + targetb) / 2;

    return (targetr << 0) | (targetg << 8) | (targetb << 16) | 0xff000000;
}

unsigned int blend_mul(int source, int target)
{
    unsigned int sourcer = ((unsigned int) source >> 0) & 0xff;
    unsigned int sourceg = ((unsigned int) source >> 8) & 0xff;
    unsigned int sourceb = ((unsigned int) source >> 16) & 0xff;
    unsigned int targetr = ((unsigned int) target >> 0) & 0xff;
    unsigned int targetg = ((unsigned int) target >> 8) & 0xff;
    unsigned int targetb = ((unsigned int) target >> 16) & 0xff;

    targetr = (sourcer * targetr) >> 8;
    targetg = (sourceg * targetg) >> 8;
    targetb = (sourceb * targetb) >> 8;

    return (targetr << 0) | (targetg << 8) | (targetb << 16) | 0xff000000;
}

unsigned int blend_add(int source, int target)
{
    unsigned int sourcer = ((unsigned int) source >> 0) & 0xff;
    unsigned int sourceg = ((unsigned int) source >> 8) & 0xff;
    unsigned int sourceb = ((unsigned int) source >> 16) & 0xff;
    unsigned int targetr = ((unsigned int) target >> 0) & 0xff;
    unsigned int targetg = ((unsigned int) target >> 8) & 0xff;
    unsigned int targetb = ((unsigned int) target >> 16) & 0xff;

    targetr += sourcer;
    targetg += sourceg;
    targetb += sourceb;

    if (targetr > 0xff)
        targetr = 0xff;
    if (targetg > 0xff)
        targetg = 0xff;
    if (targetb > 0xff)
        targetb = 0xff;

    return (targetr << 0) | (targetg << 8) | (targetb << 16) | 0xff000000;
}

void scaleblit()
{
    constexpr double zoom = 0.99;
    constexpr double expand = (1.0 - zoom) * 0.5;
    int yofs = 0;
    for (int i = 0; i < WINDOW_HEIGHT; i++)
    {
        for (int j = 0; j < WINDOW_WIDTH; j++)
        {
            int c = (int) ((i * zoom) + (WINDOW_HEIGHT * expand)) * WINDOW_WIDTH
                    + (int) ((j * zoom) + (WINDOW_WIDTH * expand));
            g.framebuffer[yofs + j] = blend_avg(g.framebuffer[yofs + j], g.tmp_buffer[c]);
        }
        yofs += WINDOW_WIDTH;
    }
}
void drawcircle(int x, int y, int r, int c)
{
    for (int i = 0; i < 2 * r; i++)
    {
        // vertical clipping: (top and bottom)
        if ((y - r + i) >= 0 && (y - r + i) < WINDOW_HEIGHT)
        {
            int len = (int) (sqrt(r * r - (r - i) * (r - i)) * 2);
            int xofs = x - len / 2;

            // left border
            if (xofs < 0)
            {
                len += xofs;
                xofs = 0;
            }

            // right border
            if (xofs + len >= WINDOW_WIDTH)
            {
                len -= (xofs + len) - WINDOW_WIDTH;
            }
            int ofs = (y - r + i) * WINDOW_WIDTH + xofs;

            // note that len may be 0 at this point,
            // and no pixels get drawn!
            for (int j = 0; j < len; j++)
                g.framebuffer[ofs + j] = c;
        }
    }
}

void render(Uint64 aTicks)
{

    int posx = (int) ((sin(aTicks * 0.000645234) + 1) * WINDOW_WIDTH / 2);
    int posy = (int) ((sin(aTicks * 0.000445234) + 1) * WINDOW_HEIGHT / 2);
    int posx2 = (int) ((sin(0 - aTicks * 0.000645234) + 1) * WINDOW_WIDTH / 2);
    int posy2 = (int) ((sin(0 - aTicks * 0.000445234) + 1) * WINDOW_HEIGHT / 2);
    for (int i = 0; i < WINDOW_HEIGHT; i++)
    {
        for (int j = 0; j < WINDOW_WIDTH; j++)
        {
            int lut = gLut[(i + posy) * WINDOW_WIDTH * 2 + j + posx]
                      - gLut[(i + posy2) * WINDOW_WIDTH * 2 + j + posx2];
            int mask = gMask[(i + posy) * WINDOW_WIDTH * 2 + j + posx];
            int mask2 = gMask[(i + posy2) * WINDOW_WIDTH * 2 + j + posx2];

            g.framebuffer[j + i * WINDOW_WIDTH] =
                blend_avg(blend_avg(gTexture[((lut + aTicks / 32) & 0xff)
                                             + (((lut >> 8) + aTicks / 8) & 0xff) * 256],
                                    mask),
                          mask2);

            // TODO : suggestions at https://solhsa.com/gp2/ch07.html
        }
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

bool init_sdl()
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
    if (!init_sdl())
        return -1;

    init_gfx();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loop, 0, 1);
#else
    while (!g.done)
        loop();
#endif

    return 0;
}
