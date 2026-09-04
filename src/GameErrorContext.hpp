#pragma once

#include <SDL2/SDL.h>
#include <cstddef>
#include <fstream>

#include "FileSystem.hpp"

struct GameErrorContext
{
    char m_Buffer[8192];
    // char *m_BufferEnd;
    i8 m_ShowMessageBox;
    std::ofstream *LogFile;

    GameErrorContext()
    {
        m_ShowMessageBox = false;
        LogFile = new std::ofstream("ux0:data/th07/log.txt", std::ios::out);
        if (LogFile->is_open())
        {
            Log("東方動作記録 --------------------------------------------- \n");
        }
        else
        {
            Fatal("ux0:data/th07/log.txt");
            LogFile->close();
            delete LogFile;
            LogFile = nullptr;
        }
    }

    ~GameErrorContext()
    {
        Flush();
        if (LogFile)
        {
            LogFile->close();
            delete LogFile;
            LogFile = nullptr;
        }
    }

    const char *Fatal(const char *fmt, ...);
    const char *Log(const char *fmt, ...);

    void Flush()
    {
        if (LogFile)
        {
            this->Log("---------------------------------------------------------- \n");
            LogFile->flush();
        }
    }
};
extern GameErrorContext g_GameErrorContext;
