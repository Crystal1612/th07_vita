#include "FileSystem.hpp"

#include <cstdio>

#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "pbg4/Pbg4Archive.hpp"

u32 g_LastFileSize;

u8 *FileSystem::OpenFile(const char *filepath, i32 isExternalResource)
{
    Supervisor::DebugPrint("FileSystem::OpenFile %s %d\n", filepath,isExternalResource);
    FILE *file;
    u8 *buf;
    u32 fsize;
    const char *filename;

    if (!isExternalResource)
    {
        filename = strrchr(filepath, '\\');
        if (!filename)
        {
            filename = filepath;
        }
        else
        {
            filename++;
        }

        filename = strrchr(filename, '/');
        if (!filename)
        {
            filename = filepath;
        }
        else
        {
            filename++;
        }
        fsize = g_Pbg4Archive.GetEntrySize(filename);
        g_LastFileSize = fsize;
        if (fsize == 0)
        {
            g_GameErrorContext.Fatal("error : %s is not found in arcfile.\n", filename);
            return NULL;
        }
        if (fsize != 0)
        {
            Supervisor::DebugPrint("%s Decode ... \n", filename);
            buf = (u8 *)malloc(fsize);
            if (!buf)
            {
                return NULL;
            }

            g_Pbg4Archive.ReadDecompressEntry(filename, buf);
            return buf;
        }
    }

    char psvPath[256] = "ux0:data/th07/";
    strcat(psvPath, filepath);
    Supervisor::DebugPrint("%s Load ... \n", psvPath);
    file = fopen(psvPath, "rb");
    if (!file)
    {
        Supervisor::DebugPrint("error : %s is not found.\n", psvPath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    buf = (u8 *)malloc(fsize);
    if (!buf)
    {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);
    if (fread(buf, 1, fsize, file) != fsize)
    {
        fclose(file);
        return NULL;
    }
    g_LastFileSize = fsize;
    fclose(file);
    return buf;
}

i32 FileSystem::CheckFileExists(const char *file)
{
    char psvPath[256] = "ux0:data/th07/";
    strcat(psvPath, file);
    Supervisor::DebugPrint("FileSystem::CheckFileExists %s", psvPath);
    FILE *fp;

    fp = fopen(psvPath, "rb");
    if (fp)
    {
        fclose(fp);
        return true;
    }
    return false;
}

i32 FileSystem::WriteDataToFile(const char *filename, const void *out, u32 bytesToWrite)
{
    char psvPath[256] = "ux0:data/th07/";
    strcat(psvPath, filename);
    Supervisor::DebugPrint("FileSystem::WriteDataToFile %s", psvPath);
    FILE *file;
    u32 bytesWritten;

    file = fopen(psvPath, "wb");
    if (!file)
    {
        Supervisor::DebugPrint("error : %s write error\n", psvPath);
        return -1;
    }

    bytesWritten = fwrite(out, 1, bytesToWrite, file);
    if (bytesToWrite != bytesWritten)
    {
        fclose(file);
        Supervisor::DebugPrint("error : %s write error\n", psvPath);
        return -2;
    }
    fclose(file);
    Supervisor::DebugPrint("%s write ...\n", psvPath);
    return 0;
}
