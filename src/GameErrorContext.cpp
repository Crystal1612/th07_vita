#include "GameErrorContext.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vitasdk.h>

GameErrorContext g_GameErrorContext;

const char *GameErrorContext::Log(const char *fmt, ...)
{
    memset(m_Buffer, 0, 8192);
    va_list args;
    va_start(args, fmt);
    vsprintf(m_Buffer, fmt, args);
    if (LogFile)
    {
        *LogFile << m_Buffer;
    }
    va_end(args);
    sceClibPrintf("%s\n", m_Buffer);
    return fmt;
}

const char *GameErrorContext::Fatal(const char *fmt, ...)
{
    memset(m_Buffer, 0, 8192);

    va_list args;
    va_start(args, fmt);
    vsprintf(m_Buffer, fmt, args);
    if (LogFile)
    {
        *LogFile << m_Buffer;
    }
    va_end(args);
    this->m_ShowMessageBox = true;
    if (this->m_ShowMessageBox)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "log", this->m_Buffer, NULL);
    }
    sceClibPrintf("%s\n", m_Buffer);
    return fmt;
}
