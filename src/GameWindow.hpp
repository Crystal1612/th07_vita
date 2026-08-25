#pragma once

#include <SDL2/SDL_video.h>

#include "ZunResult.hpp"
#include "inttypes.hpp"

enum RenderResult
{
    RENDER_RESULT_EXIT_SUCCESS_2 = -1,
    RENDER_RESULT_KEEP_RUNNING = 0,
    RENDER_RESULT_EXIT_SUCCESS = 1,
    RENDER_RESULT_EXIT_ERROR = 2
};

struct GameWindow
{
    static i32 ChecksumExecutable();
    static ZunResult CreateGameWindow();
    static ZunResult InitInterface();
    static ZunResult InitRendering();
    static void Present();
    RenderResult Render();
    static void ResetRenderState();

    SDL_Window *window;
    i32 isAppClosing;
    i32 isAppActive;
    i32 isAppInactive;
    i8 curFrame;
    // pad 3
    i64 frequency;
    bool usesRelativePath;
};

extern GameWindow g_GameWindow;
