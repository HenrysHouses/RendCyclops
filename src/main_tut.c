#include "SDL3/SDL_log.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_surface.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#ifndef SDL_PROP_SURFACE_FLIP_NUMBER
#define SDL_PROP_SURFACE_FLIP_NUMBER "SDL.surface.flip"
#endif
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "SDL3_image/SDL_image.h"

#define SDL_BLENDMODE_ADD                                                      \
  0x00000002u /**< additive blending: dstRGB = (srcRGB * srcA) + dstRGB, dstA  \
                 = dstA */
#define TREECOUNT 64

  float gTreeCoord[TREECOUNT * 2];
  // Look-up table
unsigned short *gLut;
// Texture
// int *gTexture;

int *gFrameBuffer;
Uint64 *gFrameLimit;
int *gTempBuffer;
SDL_Window *gSDLWindow;
SDL_Renderer *gSDLRenderer;
SDL_Texture *gSDLTexture;
SDL_Surface *gLoadTexure;
SDL_Texture *gTexure;
static int gDone;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

bool update() {
  SDL_Event e;
  if (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT) {
      return false;
    }
    if (e.type == SDL_EVENT_KEY_UP && e.key.key == SDLK_ESCAPE) {
      return false;
    }
  }

  char *pix;
  int pitch;

  SDL_LockTexture(gSDLTexture, NULL, (void **)&pix, &pitch);
  for (int i = 0, sp = 0, dp = 0; i < WINDOW_HEIGHT;
       i++, dp += WINDOW_WIDTH, sp += pitch)
    memcpy(pix + sp, gFrameBuffer + dp, WINDOW_WIDTH * 4);

  SDL_UnlockTexture(gSDLTexture);
  SDL_RenderTexture(gSDLRenderer, gSDLTexture, NULL, NULL);
  SDL_RenderPresent(gSDLRenderer);
  SDL_Delay(1);
  return true;
}

unsigned int blend_avg(int source, int target) {
  unsigned int sourcer = ((unsigned int)source >> 0) & 0xff;
  unsigned int sourceg = ((unsigned int)source >> 8) & 0xff;
  unsigned int sourceb = ((unsigned int)source >> 16) & 0xff;
  unsigned int targetr = ((unsigned int)target >> 0) & 0xff;
  unsigned int targetg = ((unsigned int)target >> 8) & 0xff;
  unsigned int targetb = ((unsigned int)target >> 16) & 0xff;

  targetr = (sourcer + targetr) / 2;
  targetg = (sourceg + targetg) / 2;
  targetb = (sourceb + targetb) / 2;

  return (targetr << 0) | (targetg << 8) | (targetb << 16) | 0xff000000;
}

unsigned int blend_mul(int source, int target) {
  unsigned int sourcer = ((unsigned int)source >> 0) & 0xff;
  unsigned int sourceg = ((unsigned int)source >> 8) & 0xff;
  unsigned int sourceb = ((unsigned int)source >> 16) & 0xff;
  unsigned int targetr = ((unsigned int)target >> 0) & 0xff;
  unsigned int targetg = ((unsigned int)target >> 8) & 0xff;
  unsigned int targetb = ((unsigned int)target >> 16) & 0xff;

  targetr = (sourcer * targetr) >> 8;
  targetg = (sourceg * targetg) >> 8;
  targetb = (sourceb * targetb) >> 8;

  return (targetr << 0) | (targetg << 8) | (targetb << 16) | 0xff000000;
}

unsigned int blend_add(int source, int target) {
  unsigned int sourcer = ((unsigned int)source >> 0) & 0xff;
  unsigned int sourceg = ((unsigned int)source >> 8) & 0xff;
  unsigned int sourceb = ((unsigned int)source >> 16) & 0xff;
  unsigned int targetr = ((unsigned int)target >> 0) & 0xff;
  unsigned int targetg = ((unsigned int)target >> 8) & 0xff;
  unsigned int targetb = ((unsigned int)target >> 16) & 0xff;

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

Uint8 blend_alpha(Uint8 pixel, Uint8 background, Uint8 alpha) {
  return (pixel * alpha) + (background * (1 - alpha));
}

void scaleblit() {
  int yofs = 0;
  for (int i = 0; i < WINDOW_HEIGHT; i++) {
    for (int j = 0; j < WINDOW_WIDTH; j++) {
      int c = (int)((i * 0.95) + (WINDOW_HEIGHT * 0.025)) * WINDOW_WIDTH +
              (int)((j * 0.95) + (WINDOW_WIDTH * 0.025));
      gFrameBuffer[yofs + j] =
          blend_avg(gFrameBuffer[yofs + j], gTempBuffer[c]);
    }
    yofs += WINDOW_WIDTH;
  }
}

#define PI 3.1415926535897932384626433832795

void drawcircle(int x, int y, int r, int c) {
  for (int i = 0; i < 2 * r; i++) {
    // vertical clipping: (top and bottom)
    if ((y - r + i) >= 0 && (y - r + i) < WINDOW_HEIGHT) {
      int len = (int)(sqrt(r * r - (r - i) * (r - i)) * 2);
      int xofs = x - len / 2;

      // left border
      if (xofs < 0) {
        len += xofs;
        xofs = 0;
      }

      // right border
      if (xofs + len >= WINDOW_WIDTH) {
        len -= (xofs + len) - WINDOW_WIDTH;
      }
      int ofs = (y - r + i) * WINDOW_WIDTH + xofs;

      // note that len may be 0 at this point,
      // and no pixels get drawn!
      for (int j = 0; j < len; j++)
        gFrameBuffer[ofs + j] = c;
    }
  }
}

void putpixel(int x, int y, int color) {
  if (x < 0 || y < 0 || x >= WINDOW_WIDTH || y >= WINDOW_HEIGHT) {
    return;
  }
  gFrameBuffer[y * WINDOW_WIDTH + x] = color;
}

const unsigned char sprite[] = {
    0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0};

void drawsprite(int x, int y, unsigned int color) {
  int i, j, c, yofs;
  yofs = y * WINDOW_WIDTH + x;
  for (i = 0, c = 0; i < 16; i++) {
    for (j = 0; j < 16; j++, c++) {
      if (sprite[c]) {
        gFrameBuffer[yofs + j] = color;
      }
    }
    yofs += WINDOW_WIDTH;
  }
}

void drawTexture(SDL_Texture *tex, float rotation) {

  SDL_FRect dst;
  if (rotation == 90.0f || rotation == 270.0f) {
    // Use a pre-rotated destination rectangle
    dst.x = (tex->h - tex->w) / 2.0f;
    dst.y = (tex->w - tex->h) / 2.0f;
    dst.w = (float)tex->w;
    dst.h = (float)tex->h;
  } else {
    dst.x = 0.0f;
    dst.y = 0.0f;
    dst.w = (float)tex->w;
    dst.h = (float)tex->h;
  }
  SDL_FlipMode flip = (SDL_FlipMode)SDL_GetNumberProperty(
      SDL_GetSurfaceProperties(gLoadTexure), SDL_PROP_SURFACE_FLIP_NUMBER,
      SDL_FLIP_NONE);
  SDL_RenderTextureRotated(gSDLRenderer, tex, NULL, &dst, rotation, NULL, flip);

  SDL_RenderPresent(gSDLRenderer);
}

void drawImageSurface(SDL_Surface *image, int x, int y) {
  if (SDL_MUSTLOCK(image)) {
    SDL_LockSurface(image);
  }

  Uint8 *r = malloc(8);
  Uint8 *g = malloc(8);
  Uint8 *b = malloc(8);
  Uint8 *a = malloc(8);

  Uint32 *dst = (Uint32 *)image->pixels;

  for (int y = 0; y < image->h; y++) {
    Uint32 *row = (Uint32 *)((Uint8 *)image->pixels + y * image->pitch);
    int yofs = y * WINDOW_WIDTH + x;

    for (int x = 0; x < image->w; x++) {
      SDL_GetRGBA(row[x], SDL_GetPixelFormatDetails(image->format), NULL, r, g,
                  b, a);
      Uint32 blendCol = blend_alpha(row[x], gTempBuffer[yofs + x], *a);
      gFrameBuffer[yofs + x] = row[x];
    }
  }

  if (SDL_MUSTLOCK(image)) {
    SDL_UnlockSurface(image);
  }
}

void init() {
    gLut = malloc(16 * WINDOW_WIDTH * WINDOW_HEIGHT);

  int i,j;
  for (i = 0; i < WINDOW_HEIGHT; i++)
  {
    for (j = 0; j < WINDOW_WIDTH; j++)
    {
      int xdist = j - (WINDOW_WIDTH / 2);
      int ydist = i - (WINDOW_HEIGHT / 2);

      int distance = (int)sqrt((float)(xdist * xdist + ydist * ydist));

      if (distance > 0) 
        distance = (64 * 256 / distance) & 0xff;

      int angle = (int)(((atan2((float)xdist, (float)ydist) / PI) + 1.0f) * 128);

      gLut[i * WINDOW_WIDTH + j] = (distance << 8) + angle;
    }
  }
  // srand(0x7aa7);
  // int i;
  // for (i = 0; i < TREECOUNT; i++) {
  //   int x = rand();
  //   int y = rand();
  //   gTreeCoord[i * 2 + 0] = ((x % 10000) - 5000) / 1000.0f;
  //   gTreeCoord[i * 2 + 1] = ((y % 10000) - 5000) / 1000.0f;
  // }
  // gTempBuffer = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(int));
  // for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
  //   gFrameBuffer[i] = 0xff000000;
  //   gTempBuffer[i] = 0xff000000;
  // }

  // for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
  //   gFrameBuffer[i] = 0xff000000;
  //
  // for (int i = 0; i < WINDOW_WIDTH; i++) {
  //   int p = (int)((sin((i + 3247) * 0.02) * 0.3 + sin((i + 2347) * 0.04) *
  //   0.1 +
  //                  sin((i + 4378) * 0.01) * 0.6) *
  //                     100 +
  //                 (WINDOW_HEIGHT * 2 / 3));
  //   int pos = p * WINDOW_WIDTH + i;
  //   for (int j = p; j < WINDOW_HEIGHT; j++) {
  //     gFrameBuffer[pos] = 0xff007f00;
  //     pos += WINDOW_WIDTH;
  //   }
  // }
}

void newsnow(Uint64 aTicks) {
  for (int i = 0; i < 8; i++) {
    Uint64 t = WINDOW_WIDTH - 2 +
               ((sin(aTicks * 0.001) / 2) + 1) * (10 - WINDOW_WIDTH - 2);
    // printf("%d\n", t);
    gFrameBuffer[rand() % (WINDOW_WIDTH - t - 2) + 10 +
                 (WINDOW_WIDTH - t - 2)] = 0xffffffff;
  }
}

void snowfall() {
  for (int j = WINDOW_HEIGHT - 2; j >= 0; j--) {
    int ypos = j * WINDOW_WIDTH;
    for (int i = 1; i < WINDOW_WIDTH - 1; i++) {
      if (gFrameBuffer[ypos + i] == 0xffffffff) {
        if (gFrameBuffer[ypos + i + WINDOW_WIDTH] == 0xff000000) {
          gFrameBuffer[ypos + i + WINDOW_WIDTH] = 0xffffffff;
          gFrameBuffer[ypos + i] = 0xff000000;
        } else if (gFrameBuffer[ypos + i + WINDOW_WIDTH - 1] == 0xff000000) {
          gFrameBuffer[ypos + i + WINDOW_WIDTH - 1] = 0xffffffff;
          gFrameBuffer[ypos + i] = 0xff000000;
        } else if (gFrameBuffer[ypos + i + WINDOW_WIDTH + 1] == 0xff000000) {
          gFrameBuffer[ypos + i + WINDOW_WIDTH + 1] = 0xffffffff;
          gFrameBuffer[ypos + i] = 0xff000000;
        }
      }
    }
  }
}

void render(Uint64 aTicks) {
  int i, j;
  for (i = 0; i < WINDOW_HEIGHT; i++)
  {
    for (j = 0; j < WINDOW_WIDTH; j++)
    {
      int lut = gLut[(i) * WINDOW_WIDTH + j];
      gFrameBuffer[(j) + (i) * WINDOW_WIDTH] = lut | 0xff000000;
    }
  }
  // // Clear the screen with a green color
  // for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
  //   gFrameBuffer[i] = 0xff005f00;
  //
  // float pos_x = (float)sin(aTicks * 0.00037234) * 2;
  // float pos_y = (float)cos(aTicks * 0.00057234) * 2;
  //
  // float shadow_x = (float)sin(aTicks * 0.0002934872) * 16;
  // float shadow_y = (float)cos(aTicks * 0.0001813431) * 16;
  //
  // for (int j = 0; j < TREECOUNT; j++) {
  //   float x = gTreeCoord[j * 2 + 0] + pos_x;
  //   float y = gTreeCoord[j * 2 + 1] + pos_y;
  //
  //   for (int i = 0; i < 8; i++) {
  //     drawcircle((int)(x * 200 + WINDOW_WIDTH / 2 + (i + 1) * shadow_x),
  //                (int)(y * 200 + WINDOW_HEIGHT / 2 + (i + 1) * shadow_y),
  //                (10 - i) * 5, 0xff1f4f1f);
  //   }
  // }
  //
  // for (int i = 0; i < 8; i++) {
  //   for (int j = 0; j < TREECOUNT; j++) {
  //     float x = gTreeCoord[j * 2 + 0] + pos_x;
  //     float y = gTreeCoord[j * 2 + 1] + pos_y;
  //     drawcircle((int)(x * (200 + i * 4) + WINDOW_WIDTH / 2),
  //                (int)(y * (200 + i * 4) + WINDOW_HEIGHT / 2), (9 - i) * 5,
  //                (i * 0x030906 + 0x1f671f) | 0xff000000);
  //   }
  // }
  // srand(0x7aa7);
  // int i;
  // for (i = 0; i < TREECOUNT; i++) {
  //   int x = rand();
  //   int y = rand();
  //   gTreeCoord[i * 2 + 0] = ((x % 10000) - 5000) / 1000.0f;
  //   gTreeCoord[i * 2 + 1] = ((y % 10000) - 5000) / 1000.0f;
  // }
  // if (*gFrameLimit >= aTicks) {
  //     return;
  // }
  //
  // *gFrameLimit = aTicks + 100;

  // for (int i = 0; i < 128; i++) {
  //   int d = (int)aTicks + i * 4;
  //   drawsprite(
  //       (int)(WINDOW_WIDTH / 2 +
  //             sin(d * 0.0034) * sin(d * 0.0134) * (WINDOW_WIDTH / 2 - 20)),
  //       (int)(WINDOW_HEIGHT / 2 +
  //             sin(d * 0.0033) * sin(d * 0.0234) * (WINDOW_HEIGHT / 2 - 20)),
  //       ((int)(sin((aTicks * 0.2 + i) * 0.234897) * 127 + 128) << 16) |
  //           ((int)(sin((aTicks * 0.2 + i) * 0.123489) * 127 + 128) << 8) |
  //           ((int)(sin((aTicks * 0.2 + i) * 0.312348) * 127 + 128) << 0));
  // }

  // drawTexture(gTexure, aTicks);
  // memcpy(gTempBuffer, gFrameBuffer, sizeof(int) * WINDOW_WIDTH *
  // WINDOW_HEIGHT);
  //
  // scaleblit();

  // drawImageSurface(gLoadTexure, WINDOW_WIDTH/4, WINDOW_HEIGHT/4);
  // newsnow(aTicks);
  // snowfall();
  // for (int i = 0, c = 0; i < WINDOW_HEIGHT; i++) {
  //   for (int j = 0; j < WINDOW_WIDTH; j++, c++) {
  //     gFrameBuffer[c] = 0xff000000;
  //   }
  // }

  // for (int i = 0; i < 128; i++) {
  //   drawsprite(
  //       (int)((WINDOW_WIDTH / 2) +
  //             sin((aTicks + i * 10) * 0.003459734) * (WINDOW_WIDTH / 2 -
  //             16)),
  //       (int)((WINDOW_HEIGHT / 2) +
  //             sin((aTicks + i * 10) * 0.003345973) * (WINDOW_HEIGHT / 2 -
  //             16)),
  //       ((int)(sin((aTicks * 0.2 + i) * 0.234897) * 127 + 128) << 16) |
  //           ((int)(sin((aTicks * 0.2 + i) * 0.123489) * 127 + 128) << 8) |
  //           ((int)(sin((aTicks * 0.2 + i) * 0.312348) * 127 + 128) << 0) |
  //           0xff000000);
  // }
  //
  // drawImageSurface(gLoadTexure, WINDOW_WIDTH/4, WINDOW_HEIGHT/4);
  //   for (int j = 0; j < 100; j++)
  //     putpixel(j + (aTicks % WINDOW_WIDTH), i + (aTicks % WINDOW_HEIGHT),
  //     0xffff0000);
}

void loop() {
  if (!update()) {
    gDone = 1;
#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
#endif
  } else {
    render(SDL_GetTicks());
  }
}

int main(int argc, char **argv) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    SDL_Log("SDL init failed\n");
    return -1;
  }

  gFrameLimit = malloc(64);
  gFrameBuffer = (int *)malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(int));
  gSDLWindow = SDL_CreateWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  gSDLRenderer = SDL_CreateRenderer(gSDLWindow, NULL);
  gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888,
                                  SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH,
                                  WINDOW_HEIGHT);

  if (!gFrameBuffer || !gSDLWindow || !gSDLRenderer || !gSDLTexture) {
    SDL_Log("SDL window, renderer or texture error\n");
    return -1;
  }

  gLoadTexure = IMG_Load("C:\\Users\\Henri\\repos\\RendCyclops\\hello.png");
  gTexure = SDL_CreateTextureFromSurface(gSDLRenderer, gLoadTexure);
  // drawImageSurface(gLoadTexure, 500, 500);
  gDone = 0;
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(loop, 0, 1);
#else
  init();
  while (!gDone) {
    loop();
  }
#endif

  SDL_DestroyTexture(gSDLTexture);
  SDL_DestroyRenderer(gSDLRenderer);
  SDL_DestroyWindow(gSDLWindow);
  SDL_Quit();

  return 0;
}
