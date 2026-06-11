#include <glad/gl.h>

#include "SDL3/SDL.h"
#include "SDL3/SDL_log.h"
// #include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_stdinc.h"
// #include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"

#include <math.h>
#include <stdio.h>
// #include <stdlib.h>

#ifndef SDL_PROP_SURFACE_FLIP_NUMBER
#define SDL_PROP_SURFACE_FLIP_NUMBER "SDL.surface.flip"
#endif
#include "SDL3/SDL_main.h"

// int *gFrameBuffer;
// Uint64 *gFrameLimit;
// int *gTempBuffer;

SDL_Window *gSDLWindow;
SDL_GLContext gGLContext;
SDL_Renderer *gSDLRenderer;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

bool Update(Uint64 time, Uint64 deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            /* fallthrough */
        case SDL_EVENT_QUIT:
            printf("exit code\n");
            return false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE) {
                printf("esc key exit code\n");
                return false;
            }
            break;
        default:
            break;
        }
    }

    printf("time: %llu, delta time: %llu\n", time, deltaTime);

    glClearColor(0.7f, 0.9f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(gSDLWindow);
    SDL_Delay(1);
    return true;
}

// void init()
// {
// gTempBuffer = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(int));
// for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
//     gFrameBuffer[i] = 0xff000000;
//     gTempBuffer[i] = 0xff000000;
// }
//
// for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
//     gFrameBuffer[i] = 0xff000000;
// }

int main(int argc, char **argv)
{
    // code without checking for errors
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL init failed\n");
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // gFrameBuffer = (int *)malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(int));
    // gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888,
    //                                 SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH,
    //                                 WINDOW_HEIGHT);

    SDL_CreateWindowAndRenderer(
        "RendCyclops - OpenGL + SDL3",
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL,
        &gSDLWindow, &gSDLRenderer);

    if (!gSDLWindow || !gSDLRenderer) {
        SDL_Log("SDL window, renderer or texture error\n");
        return -1;
    }

    gGLContext = SDL_GL_CreateContext(gSDLWindow);

    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);

    if (version == 0) {
        printf("Failed to initialize OpenGL context\n");
        return -1;
    }

    printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    Uint64 time = 0;
    Uint64 prevTime = 0;
    bool exit = false;
    // init();
    while (!exit) {
        prevTime = time;
        time = SDL_GetTicks();
        exit = !Update(time, time - prevTime);
    }

    SDL_GL_DestroyContext(gGLContext);
    SDL_DestroyRenderer(gSDLRenderer);
    SDL_DestroyWindow(gSDLWindow);
    SDL_Quit();

    printf("Exited normally\n");
    return 0;

    // // load texture
    // gLoadTexure = IMG_Load("C:\\Users\\Henri\\repos\\RendCyclops\\hello.png");
    // gTexure = SDL_CreateTextureFromSurface(gSDLRenderer, gLoadTexure);
    //
}
