#include "Pbg4File.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>

#include "../inttypes.hpp"

// GLOBAL: TH07 0x00495120
const u32 g_SeekModes[3] = {0, 1, 2};

// would it really not have been simpler to just type the letter where its used
// GLOBAL: TH07 0x0049ea70
const char *g_AccessModes[3] = {
    // STRING: TH07 0x00495244
    "r",
    // STRING: TH07 0x00495240
    "w",
    // STRING: TH07 0x0049523c
    "a",
};

// FUNCTION: TH07 0x0045e550
Pbg4File::Pbg4File()
{
    this->file = NULL;
    this->access = 0;
}

// FUNCTION: TH07 0x0045e5c0
Pbg4File::~Pbg4File()
{
    Close();
}

// FUNCTION: TH07 0x0045e620
bool Pbg4File::Open(const char *path, const char *mode)
{
    u32 local_118;
    char local_114[264];
    i32 local_c;
    const char *local_8;

    local_c = 0;
    this->Close();
    for (local_8 = mode; *local_8 != '\0'; ++local_8)
    {
        if (*local_8 == 'r')
        {
            this->access = "rb";
            local_118 = 3;
            break;
        }
        if (*local_8 == 'w')
        {
            remove(path);
            this->access = "wb";
            local_118 = 2;
            break;
        }
        if (*local_8 == 'a')
        {
            local_c = 1;
            this->access = "ab";
            local_118 = 4;
            break;
        }
    }
    if (*local_8 == '\0')
    {
        return false;
    }
    else
    {
        GetFullPath(local_114, path);
        this->file = fopen(local_114, this->access);
        if (!this->file)
        {
            return false;
        }

        if (local_c != 0)
        {
            fseek(this->file, 0, SEEK_END);
        }
        return true;
    }
}

// FUNCTION: TH07 0x0045e770
void Pbg4File::Close()
{
    if (this->file)
    {
        fclose(this->file);
        this->file = NULL;
        this->access = 0;
    }
}

// FUNCTION: TH07 0x0045e7b0
u32 Pbg4File::Read(void *data, u32 len)
{
    u32 local_8;

    local_8 = 0;
    if (!this->access || strcmp(this->access, "rb") != 0)
    {
        return 0;
    }

    local_8 = fread(data, 1, len, this->file);
    return local_8;
}

// FUNCTION: TH07 0x0045e800
bool Pbg4File::Write(void *data, u32 len)
{
    u32 local_8;

    local_8 = 0;
    if (!this->access || strcmp(this->access, "wb") != 0)
    {
        return false;
    }

    local_8 = fwrite(data, 1, len, this->file);
    return len == local_8;
}

// FUNCTION: TH07 0x0045e850
u32 Pbg4File::Tell()
{
    if (!this->file)
    {
        return 0;
    }
    else
    {
        return ftell(this->file);
    }
}

// FUNCTION: TH07 0x0045e880
u32 Pbg4File::GetSize()
{
    if (!this->file)
    {
        return 0;
    }
    else
    {
        long cur = ftell(this->file);
        fseek(this->file, 0, SEEK_END);
        u32 size = ftell(this->file);
        fseek(this->file, cur, SEEK_SET);
        return size;
    }
}

// FUNCTION: TH07 0x0045e8b0
bool Pbg4File::Seek(u32 offset, u32 seekFrom)
{
    if (!this->file)
    {
        return false;
    }

    fseek(this->file, offset, seekFrom);
    return true;
}

// FUNCTION: TH07 0x0045e8f0
void *Pbg4File::ReadRemaining(u32 max)
{
    void *hMem;
    u32 DVar2;
    u32 DVar3;

    if (!this->access || strcmp(this->access, "rb") != 0)
    {
        return NULL;
    }

    DVar2 = this->GetSize();
    if (DVar2 > max)
    {
        return NULL;
    }

    hMem = calloc(1, DVar2);
    if (!hMem)
    {
        return NULL;
    }

    DVar3 = this->Tell();
    if (!this->Seek(DVar3, g_SeekModes[0]))
    {
        return NULL;
    }

    if (this->Read(hMem, DVar2) == 0)
    {
        if (hMem)
        {
            free(hMem);
            hMem = NULL;
        }
        return NULL;
    }

    this->Seek(DVar3, g_SeekModes[0]);
    return hMem;
}

// FUNCTION: TH07 0x0045e9d0
void Pbg4File::GetFullPath(char *out, const char *filename)
{
    if (strchr(filename, ':') != NULL)
    {
        strcpy(out, filename);
    }
    else
    {
        GetModuleFileNameA(NULL, out, 0x104);
        char *pcVar2 = strrchr(out, '\\');
        if (!pcVar2)
        {
            strcpy(out, "");
        }
        pcVar2[1] = '\0';
        strcat(out, filename);
    }
}
