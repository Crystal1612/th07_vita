//-----------------------------------------------------------------------------
// File: dsutil.hpp
//
// Desc:
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include <dsound.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <windows.h>

#include "inttypes.hpp"

//-----------------------------------------------------------------------------
// Classes used by this header
//-----------------------------------------------------------------------------
class CSoundManager;
class CSound;
class CStreamingSound;
class CWaveFile;

//-----------------------------------------------------------------------------
// Typing macros
//-----------------------------------------------------------------------------
#define WAVEFILE_READ 1
#define WAVEFILE_WRITE 2

#define DSUtil_StopSound(s) \
    {                       \
        if (s)              \
            s->Stop();      \
    }
#define DSUtil_PlaySound(s) \
    {                       \
        if (s)              \
            s->Play(0, 0);  \
    }
#define DSUtil_PlaySoundLooping(s)       \
    {                                    \
        if (s)                           \
            s->Play(0, DSBPLAY_LOOPING); \
    }

struct ThBgmFormat
{
    char name[16];
    i32 startOffset;
    u32 preloadAllocSize;
    i32 introLength;
    i32 totalLength;
    WAVEFORMATEX format;
    // pad 2
};

//-----------------------------------------------------------------------------
// Name: class CWaveFile
// Desc: Encapsulates reading or writing sound data to or from a wave file
//-----------------------------------------------------------------------------
class CWaveFile
{
  public:
    HMMIO h_mmio;
    MMCKINFO m_ck;
    MMCKINFO m_ckRiff;
    u32 m_dwSize;
    MMIOINFO m_mmioinfo;
    u32 m_dwFlags;
    BOOL m_bIsReadingFromMemory;
    u8 *m_pbData;
    u8 *m_pbDataCur;
    ULONG m_ulDataSize;
    HANDLE m_hWaveFile;
    ThBgmFormat *m_pzwf;

    CWaveFile();
    ~CWaveFile();

    HRESULT Open(LPCSTR strFileName, ThBgmFormat *pzwf, u32 dwFlags);
    HRESULT OpenFromMemory(u8 *pbData, ULONG ulDataSize, ThBgmFormat *pzwf,
                           u32 dwFlags);
    HRESULT Close();

    HRESULT Read(u8 *pBuffer, u32 dwSizeToRead, u32 *pdwSizeRead);
    u32 GetSize();
    HRESULT ResetFile(bool bLoop);
    HRESULT Reopen(ThBgmFormat *pzwf);

    ThBgmFormat *GetFormat()
    {
        return this->m_pzwf;
    }
};

//-----------------------------------------------------------------------------
// Name: class CSoundManager
// Desc:
//-----------------------------------------------------------------------------
class CSoundManager
{
  public:
    LPDIRECTSOUND8 pDS;

    CSoundManager();
    ~CSoundManager();

    HRESULT Initialize(HWND hWnd, u32 dwCoopLevel, u32 dwPrimaryChannels,
                       u32 dwPrimaryFreq, u32 dwPrimaryBitRate);
    LPDIRECTSOUND8 GetDirectSound()
    {
        return pDS;
    }
    HRESULT SetPrimaryBufferFormat(u32 dwPrimaryChannels, u32 dwPrimaryFreq,
                                   u32 dwPrimaryBitRate);

    HRESULT CreateStreaming(CStreamingSound **ppStreamingSound,
                            LPCSTR strWaveFileName, u32 dwCreationFlags,
                            GUID guid3DAlgorithm, u32 dwNotifyCount,
                            u32 dwNotifySize, HANDLE hNotifyEvent,
                            ThBgmFormat *pzwf);
    HRESULT CreateStreamingFromMemory(CStreamingSound **ppStreamingSound,
                                      u8 *pbData, ULONG ulDataSize,
                                      ThBgmFormat *pzwf, u32 dwCreationFlags,
                                      GUID guid3DAlgorithm, u32 dwNotifyCount,
                                      u32 dwNotifySize, HANDLE hNotifyEvent);
};

//-----------------------------------------------------------------------------
// Name: class CSound
// Desc: Encapsulates functionality of a DirectSound buffer.
//-----------------------------------------------------------------------------
// VTABLE: TH07 0x00495290
class CSound
{
  public:
    LPDIRECTSOUNDBUFFER *m_apDSBuffer;
    u32 m_dwDSBufferSize;
    CWaveFile *m_pWaveFile;
    u32 m_dwNumBuffers;
    i32 m_iCurFadeoutProgress;
    i32 m_iTotalFadeout;
    u32 m_dwIsFadingOut;
    u32 m_dwPriority;
    u32 m_dwFlags;
    u32 unused_28;
    u32 unused_2c;
    BOOL m_bIsPlaying;
    DSBUFFERDESC m_dsbd;
    CSoundManager *m_pSoundManager;

    CSound(LPDIRECTSOUNDBUFFER *apDSBuffer, u32 dwDSBufferSize,
           u32 dwNumBuffers, CWaveFile *pWaveFile);
    virtual ~CSound();

    // SYNTHETIC: TH07 0x0045d030
    // CSound::`scalar deleting destructor'

    HRESULT RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL *pbWasRestored);
    HRESULT FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB,
                                BOOL bRepeatWavIfBufferLarger);
    LPDIRECTSOUNDBUFFER GetFreeBuffer();
    LPDIRECTSOUNDBUFFER GetBuffer(u32 dwIndex);

    HRESULT Play(u32 dwPriority, u32 dwFlags);
    u32 Stop();
    HRESULT Reset();
    HRESULT Pause();
    HRESULT Unpause();
};

//-----------------------------------------------------------------------------
// Name: class CStreamingSound
// Desc: Encapsulates functionality to play a wave file with DirectSound.
//-----------------------------------------------------------------------------
// VTABLE: TH07 0x0049528c
class CStreamingSound : public CSound
{
  public:
    u32 m_dwLastPlayPos;
    u32 m_dwPlayProgress;
    u32 m_dwNextWriteOffset;
    BOOL m_bFillNextNotificationWithSilence;
    u32 m_dwNotifySize;
    HANDLE m_hNotifyEvent;
    BOOL m_bIsLocked;

    CStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer, u32 dwDSBufferSize,
                    CWaveFile *pWaveFile, u32 dwNotifySize);
    ~CStreamingSound();

    // SYNTHETIC: TH07 0x0045da80
    // CStreamingSound::`scalar deleting destructor'

    HRESULT HandleWaveStreamNotification(i32 bLoopedPlay);
    HRESULT Reset();
    HRESULT InitSoundBuffers();
    HRESULT UpdateFadeOut();

    void FadeOut(f32 duration)
    {
        this->m_dwIsFadingOut = 1;
        this->m_iCurFadeoutProgress = (i32)(duration * 60.0f);
        this->m_iTotalFadeout =
            this->m_iCurFadeoutProgress;
    }

    CWaveFile *GetWaveFile()
    {
        return this->m_pWaveFile;
    }
};

void DebugPrint(const char *fmt, ...);
