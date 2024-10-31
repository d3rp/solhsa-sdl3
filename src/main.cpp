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

constexpr int WINDOW_WIDTH = 1920 / 2;
constexpr int WINDOW_HEIGHT = 1080 / 2;

void putpixel(int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= WINDOW_WIDTH || y >= WINDOW_HEIGHT)
    {
        return;
    }
    G.framebuffer[y * WINDOW_WIDTH + x] = color;
}

constexpr int vertices_n = 512;
void init_gfx()
{
    gVtx = new Vertex[vertices_n];
    gRVtx = new Vertex[vertices_n];

    for (int i = 0; i < vertices_n; i++)
    {
        gVtx[i].x = (rand() % 32768) - 10384.0f;
        gVtx[i].y = (rand() % 32768) - 16384.0f;
        gVtx[i].z = (rand() % 32768) - 12384.0f;
        float len =
            (float) sqrt(gVtx[i].x * gVtx[i].x + gVtx[i].y * gVtx[i].y + gVtx[i].z * gVtx[i].z);
        if (len != 0)
        {
            gVtx[i].x /= len;
            gVtx[i].y /= len;
            gVtx[i].z /= len;
        }
    }
}

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
                G.framebuffer[yofs + j] = color;

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
void render(Uint64 aTicks)
{
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
        G.framebuffer[i] = 0xff000000;

    drawcircle(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, WINDOW_WIDTH / 4, 0xff6f2f2f);

    memcpy(gRVtx, gVtx, sizeof(Vertex) * vertices_n);

    double rotz = aTicks * 0.0005;
    double roty = aTicks * 0.002;

    rotate_y(roty * sin(rotz) * 0.1f);
    rotate_z(rotz);

    for (int i = 0; i < vertices_n; i++)
    {
        int c = 0xffffce1e;
        if (gRVtx[i].z < 0)
            c = 0xff7f6f3f;

        drawcircle((int) (gRVtx[i].x * (WINDOW_WIDTH / 4) + WINDOW_WIDTH / 2),
                   (int) (gRVtx[i].y * (WINDOW_WIDTH / 4) + WINDOW_HEIGHT / 2),
                   2,
                   c);

        drawcircle((int) ((WINDOW_WIDTH / 3) * sin(rotz) + WINDOW_WIDTH / 2),
                   (int) ((WINDOW_WIDTH / 3) * cos(rotz) + WINDOW_HEIGHT / 2),
                   6,
                   0xffcfcfcf);

        float cos2 = cos(rotz);
        float sin2 = sin(rotz) * sin(0.1f * rotz);
        // int c2 = -0.75f < cos2 && cos2 < 0.75f ? 0xcf7f4f4f : 0xcfcfcfcf;
        int c2 = 0x11666666;
        drawcircle((int) ((WINDOW_WIDTH / 3) * cos2 + WINDOW_WIDTH / 2),
                   (int) ((WINDOW_WIDTH / 3) * sin2 + WINDOW_HEIGHT / 2),
                   6,
                   c2);
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

    return 0;
}
