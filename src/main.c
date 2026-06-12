#include <glad/gl.h>

// #include "SDL3/SDL.h"
// #include "SDL3/SDL_rect.h"
// #include "SDL3/SDL_surface.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"
#include "SDL3_ttf/SDL_ttf.h"

#include <math.h>
#include <stdio.h>
// #include <stdlib.h>

#ifndef SDL_PROP_SURFACE_FLIP_NUMBER
#define SDL_PROP_SURFACE_FLIP_NUMBER "SDL.surface.flip"
#endif
#include "SDL3/SDL_main.h"

#define CLAY_IMPLEMENTATION
#include "viewport/View.h"
#include <clay.h>
#include <renderers/SDL3/clay_renderer_SDL3.c>

typedef struct app_state
{
    Uint64 DeltaTime;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_GLContext GLContext;
    Clay_SDL3RendererData rendererData;
} AppState;

// const int WINDOW_WIDTH = 1920 / 2;
// const int WINDOW_HEIGHT = 1080 / 2;
// int *gFrameBuffer;
// int *gTempBuffer;
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

AppState *gAppState;
SDL_Texture *gSDLTexture;

static bool resizingEventWatcher(void *data, SDL_Event *event)
{
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        SDL_Window *win = SDL_GetWindowFromID(event->window.windowID);
        if (win == (SDL_Window *)data) {
            // int *w, *h;
            // SDL_GetWindowSize(win, w, h);
            // framebuffer_size_callback(win, *w, *h);
            printf("resizing.....\n");
        }
    }
    return 0;
}

static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    TTF_Font **fonts = userData;
    TTF_Font *font = fonts[config->fontId];
    int width, height;

    TTF_SetFontSize(font, config->fontSize);
    if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return (Clay_Dimensions){ (float)width, (float)height };
}

void HandleClayErrors(Clay_ErrorData errorData)
{
    printf("%s", errorData.errorText.chars);
}

// NOTE this testing function defines a callback for handling clicks
void HandleSidebarItemClick(Clay_ElementId elementId, Clay_PointerData pointerData, void *userData)
{
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        int *itemIndex = (int *)userData;
        // Handle click - itemIndex tells you which item was clicked
        printf("Clicked item %d\n", *itemIndex);
    }
}

Clay_RenderCommandArray ClayImageSample_CreateLayout()
{
    Clay_BeginLayout();

    Clay_Sizing layoutExpand = {
        .width = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    CLAY(CLAY_ID("OuterContainer"),
         { .layout = {
               .sizing = layoutExpand,
               .padding = CLAY_PADDING_ALL(16),
               .childGap = 16 } })
    // .backgroundColor = { 250, 250, 255, 255 } })
    {
        CLAY(CLAY_ID("SideBar"),
             { .layout = {
                   .layoutDirection = CLAY_TOP_TO_BOTTOM,
                   .sizing = { .width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0) },
                   .padding = CLAY_PADDING_ALL(16),
                   .childGap = 16 },
               .backgroundColor = { 224, 215, 210, 255 } })
        {
            CLAY(CLAY_ID("ProfilePictureOuter"),
                 { .layout = {
                       .sizing = { .width = CLAY_SIZING_GROW(0) },
                       .padding = CLAY_PADDING_ALL(16),
                       .childGap = 16,
                       .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                   .backgroundColor = { 168, 66, 28, 255 } })
            {
                CLAY(CLAY_ID("ProfilePicture"),
                     { .layout = {
                           .sizing = { .width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) } },
                       .image = { .imageData = gSDLTexture } }){};

                CLAY_TEXT(CLAY_STRING("Clay - UI Library"),
                          { .fontSize = 24, .textColor = { 255, 255, 255, 255 } });
            }

            // Add clickable sidebar items
            for (int i = 0; i < 5; i++) {
                static int itemIndices[5] = { 0, 1, 2, 3, 4 };
                CLAY(CLAY_IDI("SidebarItem", i),
                     { .layout = {
                           .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                           .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50) } },
                       .backgroundColor = { 225, 138, 50, 255 } })
                {
                    Clay_OnHover(HandleSidebarItemClick, &itemIndices[i]);
                    CLAY_TEXT(CLAY_STRING("Click me"), { .fontSize = 16, .textAlignment = CLAY_TEXT_ALIGN_CENTER, .textColor = { 255, 255, 255, 255 } });
                };
            }
        }

        CLAY(CLAY_ID("MainContent"), { .layout = { .sizing = layoutExpand }, }){
            // this is the right empty container
        };
    }

    return Clay_EndLayout(gAppState->DeltaTime);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!TTF_Init()) {
        return SDL_APP_FAILURE;
    }

    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        return SDL_APP_FAILURE;
    }
    *appstate = state;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    if (!SDL_CreateWindowAndRenderer(
            "Clay Demo",
            640, 480,
            SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS,
            &state->window, &state->renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // SDL_SetWindowBordered(gSDLWindow, false);
    if (!state->window || !state->renderer) {
        SDL_Log("SDL window, renderer or texture error\n");
        return -1;
    }

    state->GLContext = SDL_GL_CreateContext(state->window);
    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);

    if (version == 0) {
        printf("Failed to initialize OpenGL context\n");
        return -1;
    }

    printf("GL %d.%d\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    SDL_SetWindowResizable(state->window, true);

    state->rendererData.textEngine = TTF_CreateRendererTextEngine(state->renderer);
    if (!state->rendererData.textEngine) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine from renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.fonts = SDL_calloc(1, sizeof(TTF_Font *));
    if (!state->rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    TTF_Font *font = TTF_OpenFont("resources/JetBrainsMonoNerdFont-Regular.ttf", 24);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.fonts[0] = font;

    /* Initialize Clay */
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena){
        .memory = SDL_malloc(totalMemorySize),
        .capacity = totalMemorySize
    };

    int width, height;
    SDL_GetWindowSize(state->window, &width, &height);
    Clay_Initialize(clayMemory, (Clay_Dimensions){ (float)width, (float)height }, (Clay_ErrorHandler){ HandleClayErrors });
    Clay_SetMeasureTextFunction(SDL_MeasureText, state->rendererData.fonts);

    // glViewport(316, 0, 100, 300);
    glViewport(0, 0, 640, 480);
    // state->demoData = ClayVideoDemo_Initialize();
    SDL_AddEventWatch(resizingEventWatcher, state->window);

    state->rendererData.renderer = state->renderer;
    *appstate = state;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = appstate;

    float mouse_x, mouse_y;

    Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

    Clay_SetPointerState(
        (Clay_Vector2){ .x = mouse_x, .y = mouse_y },
        buttons & SDL_BUTTON_LMASK);

    Clay_RenderCommandArray render_commands = (ClayImageSample_CreateLayout());

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    SDL_Clay_RenderClayCommands(&state->rendererData, &render_commands);

    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

bool Update(Uint64 time)
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

    SDL_AppIterate(gAppState);

    glClearColor(0.7f, 0.9f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(gAppState->window);
    SDL_Delay(1);
    return true;
}

int main(int argc, char **argv)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL init failed\n");
        return -1;
    }

    SDL_AppInit((void **)&gAppState, argc, argv);

    SDL_Surface *gLoadSurface = IMG_Load("C:\\Users\\Henri\\repos\\RendCyclops\\hello.png");
    gSDLTexture = SDL_CreateTextureFromSurface(gAppState->renderer, gLoadSurface);

    Uint64 time = 0;
    Uint64 prevTime = 0;
    bool exit = false;
    while (!exit) {
        prevTime = time;
        time = SDL_GetTicks();
        gAppState->DeltaTime = time - prevTime;
        exit = !Update(time);
    }

    SDL_GL_DestroyContext(gAppState->GLContext);
    SDL_DestroyRenderer(gAppState->renderer);
    SDL_DestroyWindow(gAppState->window);
    SDL_Quit();

    printf("Exited normally\n");
    return 0;
}
