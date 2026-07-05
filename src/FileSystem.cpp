#include "FileSystem.hpp"

#include <cstdio>

#include "GameErrorContext.hpp"
#include "dsutil.hpp"
#include "pbg4/Pbg4Archive.hpp"

u32 g_LastFileSize;

u8 *FileSystem::OpenFile(const char *filepath, i32 isExternalResource)
{
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
            g_GameErrorContext.Fatal("error : %s is not found in arcfile.\r\n",
                                     filename);
            return NULL;
        }
        if (fsize != 0)
        {
            DebugPrint("%s Decode ... \r\n", filename);
            buf = (u8 *)malloc(fsize);
            if (!buf)
            {
                return NULL;
            }

            g_Pbg4Archive.ReadDecompressEntry(filename, buf);
            return buf;
        }
    }
    DebugPrint("%s Load ... \r\n", filepath);
    file = fopen(filepath, "rb");
    if (!file)
    {
        DebugPrint("error : %s is not found.\r\n", filepath);
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
    FILE *fp;

    fp = fopen(file, "rb");
    if (fp)
    {
        fclose(fp);
        return true;
    }
    return false;
}

i32 FileSystem::WriteDataToFile(const char *filename, const void *out,
                                u32 bytesToWrite)
{
    FILE *file;
    u32 bytesWritten;

    file = fopen(filename, "wb");
    if (!file)
    {
        DebugPrint("error : %s write error\r\n", filename);
        return -1;
    }

    bytesWritten = fwrite(out, 1, bytesToWrite, file);
    if (bytesToWrite != bytesWritten)
    {
        fclose(file);
        DebugPrint("error : %s write error\r\n", filename);
        return -2;
    }
    fclose(file);
    DebugPrint("%s write ...\r\n", filename);
    return 0;
}
