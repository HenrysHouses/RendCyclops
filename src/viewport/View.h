#pragma once
#include "SDL3/SDL_video.h"
#include <glad/gl.h>

void framebuffer_size_callback(SDL_Window* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
