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

struct Bump
{
    // Picture
    int* picture;
    // Heightmap
    int* heightmap;
    // Lightmap
    int* lightmap;
    // Picture size
    int picture_w, picture_h;
} B;
// Bump lookup table
short* gBumpLut;

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
    SDL_FPoint mouse_pos;
    bool done { false };
    int* framebuffer { nullptr };
    unsigned int* tmp_buffer;
    SDL_Window* window { nullptr };
    SDL_Renderer* renderer { nullptr };
    SDL_Texture* texture { nullptr };
} G;

// Vertex structure
struct Vertex
{
    float x, y, z;
};

// Original vertices
Vertex* gVtx;
// Transformed vertices
Vertex* gRVtx;

struct Particle
{
    float x = 0;
    float y = 0;
    float xi = 0;
    float yi = 0;
    int live = 0;
    int gen = 0;
    int type = 0;
    int color = 0;
};

#define MAX_PARTICLES 16384
Particle gParticle[MAX_PARTICLES];
unsigned int gNextParticle = 0;

void spawn(Particle q)
{
    int n = gNextParticle % MAX_PARTICLES;
    gNextParticle++;
    gParticle[n] = q;
}

int* gBall;
int* gFrameBufferPile;
int gFrame = 0;
constexpr int WINDOW_WIDTH = 1920 / 2;
constexpr int WINDOW_HEIGHT = 1080 / 2;
void physics_tick(Uint64 aTicks)
{

    if (((aTicks / 8) % 80) == 0)
    {
        spawn({ WINDOW_WIDTH / 2,
                WINDOW_HEIGHT,
                (rand() % 256 - 128) / 32.0f,
                -6 - (rand() % 256) / 64.0f,
                100 + rand() % 20,
                0,
                0,
                0 });
    }

    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (gParticle[i].live)
        {
            if (gParticle[i].type == 0)
            {
                spawn({ gParticle[i].x,
                        gParticle[i].y,
                        (rand() % 100) / 200.0f - 0.25f,
                        (rand() % 100) / 200.0f,
                        50 + rand() % 20,
                        0,
                        1,
                        0 });
                gParticle[i].yi += 0.1f;
                gParticle[i].x += gParticle[i].xi;
                gParticle[i].y += gParticle[i].yi;
                gParticle[i].live--;
                if (!gParticle[i].live && gParticle[i].y < WINDOW_HEIGHT && gParticle[i].gen < 2)
                {
                    float x = gParticle[i].x;
                    float y = gParticle[i].y;
                    float xi = gParticle[i].xi / 2;
                    float yi = 0;
                    int gen = gParticle[i].gen + 1;

                    for (int j = 0; j < 12; j++)
                    {
                        spawn({ x, y, 0, 0, 10, 0, 2, 0 });
                        spawn({ x,
                                y,
                                xi + (rand() % 256 - 128) / 64.0f,
                                yi - (float) (rand() % 512) / 100.0f,
                                60 + rand() % 40,
                                gen,
                                0,
                                0 });
                    }
                }
            }

            if (gParticle[i].type == 1)
            {
                gParticle[i].y += gParticle[i].yi;
                gParticle[i].x += gParticle[i].xi;
                gParticle[i].live--;
            }

            if (gParticle[i].type == 2)
            {
                gParticle[i].live--;
            }
        }
    }
}
void putpixel(int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= WINDOW_WIDTH || y >= WINDOW_HEIGHT)
    {
        return;
    }
    G.framebuffer[y * WINDOW_WIDTH + x] = color;
}

constexpr int vertices_n = 512;

void newsnow()
{
    for (int i = 0; i < 8; i++)
        G.framebuffer[rand() % (WINDOW_WIDTH - 2) + 1] = 0xffffffff;
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
            if (G.framebuffer[ypos + i] < white_pxl)
                continue;

            if (G.framebuffer[ypos + i + WINDOW_WIDTH] == 0xff000000)
            {
                G.framebuffer[ypos + i + WINDOW_WIDTH] = 0xffffffff;
                G.framebuffer[ypos + i] = 0xff000000;
            }
            else if (G.framebuffer[ypos + i + WINDOW_WIDTH - 1] == 0xff000000)
            {
                G.framebuffer[ypos + i + WINDOW_WIDTH - 1] = 0xffffffff;
                G.framebuffer[ypos + i] = 0xff000000;
            }
            else if (G.framebuffer[ypos + i + WINDOW_WIDTH + 1] == 0xff000000)
            {
                G.framebuffer[ypos + i + WINDOW_WIDTH + 1] = 0xffffffff;
                G.framebuffer[ypos + i] = 0xff000000;
            }
        }
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
        if (e.type == SDL_EVENT_MOUSE_MOTION)
            G.mouse_pos = { e.motion.x, e.motion.y };
    }

    char* pix;
    int pitch;

    SDL_LockTexture(G.texture, nullptr, (void**) &pix, &pitch);
    for (int i = 0, sp = 0, dp = 0; i < WINDOW_HEIGHT; i++, dp += WINDOW_WIDTH, sp += pitch)
        memcpy(pix + sp, G.framebuffer + dp, WINDOW_WIDTH * 4);

    SDL_UnlockTexture(G.texture);
    SDL_RenderTexture(G.renderer, G.texture, nullptr, nullptr);
    SDL_RenderPresent(G.renderer);
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

void drawball(int x, int y, int c)
{
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++)
            if (gBall[i * 64 + j] != 0xff000000)
                G.framebuffer[x + j + (y + i) * WINDOW_WIDTH] =
                    blend_mul(gBall[i * 64 + j], c) | 0xff000000;
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
                G.framebuffer[ofs + j] = c;
        }
    }
}

void drawcircle_add(int x, int y, int r, int c)
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
                G.framebuffer[ofs + j] = blend_add(G.framebuffer[ofs + j], c);
        }
    }
}
void drawcircle_mul(int x, int y, int r, int c)
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
                G.framebuffer[ofs + j] = blend_mul(G.framebuffer[ofs + j], c);
        }
    }
}
void init_gfx()
{
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
        G.framebuffer[i] = 0xff000000;

    for (int i = 0; i < WINDOW_HEIGHT / 2; i++)
    {
        int c = (0xff * i) / (WINDOW_HEIGHT / 2);
        c = 0x010000 * c | 0xff000000;
        for (int j = 0; j < WINDOW_WIDTH; j++)
        {
            G.framebuffer[i * WINDOW_WIDTH + j] = c;
        }
    }
    for (int i = 0; i < 100; i++)
    {
        int y = rand() % (WINDOW_HEIGHT / 2);
        int x = rand() % WINDOW_WIDTH;
        int c = rand() % 0xff;
        c = 0x010101 * c | 0xff000000;
        G.framebuffer[y * WINDOW_WIDTH + x] = blend_add(c, G.framebuffer[y * WINDOW_WIDTH + x]);
    }
    drawcircle(WINDOW_WIDTH / 2, 100, 60, 0xffff7f00);
    // ground
    for (int i = 0; i < WINDOW_WIDTH; i++)
    {
        int h = (sin(i * 0.01) * sin(i * 0.02) * sin(i * 0.03)) * WINDOW_HEIGHT / 20
                + WINDOW_HEIGHT / 10;
        for (int j = WINDOW_HEIGHT / 2 - h; j < WINDOW_HEIGHT / 2; j++)
        {
            G.framebuffer[j * WINDOW_WIDTH + i] = 0xff114466;
        }
    }
    // trees
    for (int count = 0; count < 60; count++)
    {
        int xofs = rand() % WINDOW_WIDTH;
        int yofs = WINDOW_HEIGHT / 2 - 60 + count;
        int ht = rand() % WINDOW_HEIGHT / 10 + WINDOW_HEIGHT / 5;

        int c = rand() % 0x1f;
        c = 0x000100 * c | 0xff000000;

        for (int i = 0; i < ht; i++)
        {
            int w = (ht / 3) * (ht - i) / ht;
            int nudge = (w > 3 ? rand() % 7 - 3 : 0);
            for (int j = 0; j < w; j++)
            {
                int p = j - w / 2 + xofs + nudge;
                if (p >= 0 && p < WINDOW_WIDTH)
                    G.framebuffer[(yofs - i) * WINDOW_WIDTH + p] = c;
            }
        }
    }
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
            G.framebuffer[yofs + j] = blend_avg(G.framebuffer[yofs + j], G.tmp_buffer[c]);
        }
        yofs += WINDOW_WIDTH;
    }
}

void dist(float v)
{
    int i, j;
    int xdist[WINDOW_WIDTH];
    int ydist[WINDOW_HEIGHT];
    const int zmax = WINDOW_HEIGHT;
    int zdist[zmax];
    static int z = 0;
    int z_offset = ++z % zmax;
    int ypos = (int) (-pow(v + 0.001f * z_offset, 5) * (B.picture_h + 64));
    for (int i = 0; i < WINDOW_WIDTH; i++)
    {
        xdist[i] = zdist[z_offset];
        // xdist[i] = sin((double) (size_t) (void*) &xdist[i]);  //(int) (sin((v * i) * 0.0324857)
        // * v
        // * 32);
    }

    for (int i = 0; i < WINDOW_HEIGHT; i++)
    {
        ydist[i] = zdist[z_offset + i];
        // ydist[i] = log((float) i);  //(int) (sin((v * i) * 0.0234557) * v * 32);
    }
    for (i = 0; i < B.picture_h; i++)
    {
        for (j = 0; j < B.picture_w; j++)
        {
            int u = j + ydist[i];
            int v = i + xdist[j] + ypos;
            if (u >= 0 && u < B.picture_w && v >= 0 && v < B.picture_h)
            {
                G.framebuffer[j + i * WINDOW_WIDTH] = B.picture[u + v * B.picture_w];
            }
            else
            {
                G.framebuffer[j + i * WINDOW_WIDTH] = B.heightmap[j + i * B.picture_w];
            }
        }
    }
}

void rotate_z(double angle)
{
    float ca = (float) cos(angle);
    float sa = (float) sin(angle);
    for (int i = 0; i < vertices_n; i++)
    {
        float x = gRVtx[i].x * ca - gRVtx[i].y * sa;
        float y = gRVtx[i].x * sa + gRVtx[i].y * ca;
        gRVtx[i].x = x;
        gRVtx[i].y = y;
    }
}
void rotate_y(double angle)
{
    float ca = (float) cos(angle);
    float sa = (float) sin(angle);
    for (int i = 0; i < vertices_n; i++)
    {
        float z = gRVtx[i].z * ca - gRVtx[i].x * sa;
        float x = gRVtx[i].z * sa + gRVtx[i].x * ca;
        gRVtx[i].z = z;
        gRVtx[i].x = x;
    }
}
void rotate_x(double angle)
{
    float ca = (float) cos(angle);
    float sa = (float) sin(angle);
    for (int i = 0; i < vertices_n; i++)
    {
        float y = gRVtx[i].y * ca - gRVtx[i].z * sa;
        float z = gRVtx[i].y * sa + gRVtx[i].z * ca;
        gRVtx[i].y = y;
        gRVtx[i].z = z;
    }
}

int gen_color(int color, int live, float scale)
{
    float a = color / 1024.0f;
    int r = sin(a) * 127 + 128;
    int g = sin(a + 2 * M_PI / 3) * 127 + 128;
    int b = sin(a + 4 * M_PI / 3) * 127 + 128;
    r = (r + 128) / 2;
    g = (g + 128) / 2;
    b = (b + 128) / 2;
    scale *= live / 100.0f;
    r *= scale;
    g *= scale;
    b *= scale;
    return (b << 16) | (g << 8) | (r << 0) | 0xff000000;
}

void render(Uint64 aTicks)
{
    static Uint64 lastTick = 0;

    for (int i = 0; i < WINDOW_HEIGHT; i++)
    {
        int c = (64 * i) / WINDOW_HEIGHT;
        c = 0x010000 * c | 0xff000000;
        for (int j = 0; j < WINDOW_WIDTH; j++)
        {
            G.framebuffer[i * WINDOW_WIDTH + j] = c;
        }
    }

    while (lastTick < aTicks)
    {
        physics_tick(lastTick);
        lastTick += 20;
    }

    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (gParticle[i].live != 0)
        {
            if (gParticle[i].type == 2)
            {
                int x = (int) gParticle[i].x;
                int y = (int) gParticle[i].y;
                int c = gParticle[i].live * 4;
                c *= 0x010101;
                c |= 0xff000000;
                drawcircle_mul(x, y, 15, c);
                drawcircle(x, y, 12, c);
            }
            else
            {
                int x = (int) gParticle[i].x;
                int y = (int) gParticle[i].y;

                int c = gen_color(gParticle[i].color, gParticle[i].live, 0.1);
                drawcircle_add(x, y, 3, c);
                c = gen_color(gParticle[i].color, gParticle[i].live, 1);
                drawcircle_add(x, y, 1, c);
            }
        }
    }
}

void loop()
{
    if (!update())
    {
        G.done = true;
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

    G.framebuffer = new int[WINDOW_WIDTH * WINDOW_HEIGHT];
    G.window = SDL_CreateWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    G.renderer = SDL_CreateRenderer(G.window, nullptr);
    G.texture = SDL_CreateTexture(G.renderer,
                                  SDL_PIXELFORMAT_ABGR8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  WINDOW_WIDTH,
                                  WINDOW_HEIGHT);

    if (!G.framebuffer || !G.window || !G.renderer || !G.texture)
        return false;

    return true;
}

void destroy()
{
    // delete gFrameBufferPile;

    SDL_DestroyTexture(G.texture);
    SDL_DestroyRenderer(G.renderer);
    SDL_DestroyWindow(G.window);
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
    while (!G.done)
        loop();
#endif

    destroy();
    return 0;
}
