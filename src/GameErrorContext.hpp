#pragma once

#include <cstddef>
#include <windows.h>

#include "FileSystem.hpp"

struct GameErrorContext
{
    char m_Buffer[8192];
    char *m_BufferEnd;
    i8 m_ShowMessageBox;

    GameErrorContext()
    {
        m_BufferEnd = m_Buffer;
        m_Buffer[0] = '\0';
        m_ShowMessageBox = false;
        Log("東方動作記録 --------------------------------------------- \r\n");
    }

    const char *Fatal(const char *fmt, ...);
    const char *Log(const char *fmt, ...);

    // FUNCTION: TH07 0x00433e90
    void Flush()
    {
        if (this->m_BufferEnd != this->m_Buffer)
        {
            this->Log("---------------------------------------------------------- \r\n");
            if (this->m_ShowMessageBox)
            {
                MessageBoxA(NULL, this->m_Buffer, "log", MB_ICONERROR);
            }
            FileSystem::WriteDataToFile("./log.txt", this->m_Buffer, strlen(this->m_Buffer));
        }
    }
};
extern GameErrorContext g_GameErrorContext;
