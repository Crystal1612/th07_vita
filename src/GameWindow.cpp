#include "GameWindow.hpp"

#include <cmath>
#include <cstdio>
#include <d3d8.h>
#include <direct.h>

typedef __w64 long SHANDLE_PTR; // i dont know anymore bro

#include <shlobj.h>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"

GameWindow g_GameWindow;

HANDLE g_Mutex;

i32 g_FrameCount;

f64 g_LastFrameTime;

LARGE_INTEGER g_LastPerfCounter;

// winmain should probably be here

LRESULT __stdcall GameWindow::WindowProc(HWND hWnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return 1;
    case MM_MOM_DONE:
        if (g_Supervisor.midiOutput)
        {
            g_Supervisor.midiOutput->UnprepareHeader((LPMIDIHDR)lParam);
        }
        break;
    case WM_ACTIVATEAPP:
        g_GameWindow.isAppActive = wParam;
        if (g_GameWindow.isAppActive)
        {
            g_GameWindow.isAppInactive = 0;
        }
        else
        {
            g_GameWindow.isAppInactive = 1;
        }
        break;
    case WM_SETCURSOR:
        if (!g_Supervisor.cfg.windowed)
        {
            if (g_GameWindow.isAppInactive)
            {
                SetCursor(LoadCursorA(NULL, IDC_ARROW));
                ShowCursor(1);
            }
            else
            {
                ShowCursor(0);
                SetCursor(NULL);
            }
        }
        else
        {
            SetCursor(LoadCursorA(NULL, IDC_ARROW));
            ShowCursor(1);
        }
        return 1;
    case WM_CLOSE:
        g_GameWindow.isAppClosing = 1;
        return 1;
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

void GameWindow::Present()
{
    char snapshotPath[252];
    i32 i;

    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_AnmManager->ReleaseSurfaces();
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
        ResetRenderState();
        g_Supervisor.renderSkipFrames = 2;
    }
    g_AnmManager->TakeScreenshotIfRequested();
    if (WAS_PRESSED_RAW(TH_BUTTON_HOME))
    {
        _mkdir("snapshot");
        for (i = 0; i < 1000; i++)
        {
            sprintf(snapshotPath, "snapshot/th%.3d.bmp", i);
            if (FileSystem::CheckFileExists(snapshotPath) == 0)
            {
                break;
            }
        }
        if (i < 1000)
        {
            g_Supervisor.SnapshotScreen(snapshotPath);
        }
    }
    if (g_Supervisor.renderSkipFrames != 0)
    {
        g_Supervisor.renderSkipFrames--;
    }
}

RenderResult GameWindow::Render()
{
    f64 timeDiff;
    f64 curTime;
    f64 perfDiff;
    LARGE_INTEGER perfCounter;
    i32 chainRes;

    if (!this->isAppActive)
    {
        return RENDER_RESULT_KEEP_RUNNING;
    }

    if (this->curFrame == 0)
    {
    begin_loop:
        if ((i32)g_Supervisor.cfg.frameskipConfig <= (i32)this->curFrame)
        {
            g_Supervisor.d3dDevice->BeginScene();
            g_AnmManager->ResetVertexBuffer();
            g_Supervisor.fogEnabled = 255;
            g_Supervisor.DisableFog();
            g_Chain.RunDrawChain();
            g_AnmManager->Flush();
            g_Supervisor.d3dDevice->SetTexture(0, NULL);
            g_Supervisor.d3dDevice->EndScene();
        }

        g_AnmManager->Flush();
        g_Supervisor.viewport.X = 0;
        g_Supervisor.viewport.Y = 0;
        g_Supervisor.viewport.Width = 640;
        g_Supervisor.viewport.Height = 480;
        g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);

        chainRes = g_Chain.RunCalcChain();
        g_SoundPlayer.ProcessQueues();

        if (!chainRes)
        {
            return RENDER_RESULT_EXIT_SUCCESS;
        }
        if (chainRes == -1)
        {
            return RENDER_RESULT_EXIT_ERROR;
        }

        this->curFrame++;
    }

    if (g_Supervisor.cfg.windowed || g_Supervisor.VsyncEnabled())
    {
        if (this->curFrame != 0)
        {
            if (g_GameWindow.lpFrequency.LowPart != 0)
            {
                QueryPerformanceCounter(&perfCounter);
                perfDiff = (f64)(perfCounter.LowPart - g_LastPerfCounter.LowPart) /
                           (f64)g_GameWindow.lpFrequency.LowPart;

                if (perfDiff < 0.0)
                {
                    g_LastPerfCounter.LowPart = perfCounter.LowPart;
                    g_LastPerfCounter.HighPart = perfCounter.HighPart;
                }

                if (perfDiff >= 1.0 / 60.0 || g_GameWindow.usesRelativePath)
                {
                    while (perfDiff >= 1.0 / 60.0)
                    {
                        g_LastPerfCounter.LowPart += g_GameWindow.lpFrequency.LowPart / 60;
                        perfDiff -= 1.0 / 60.0;
                    }
                    if ((i32)g_Supervisor.cfg.frameskipConfig < (i32)this->curFrame)
                    {
                        goto LAB_00434a18;
                    }
                    goto begin_loop;
                }
            }
            else
            {
                timeBeginPeriod(1);
                curTime = (f64)timeGetTime();

                if (curTime < g_LastFrameTime)
                {
                    g_LastFrameTime = curTime;
                }

                timeDiff = fabs(curTime - g_LastFrameTime);
                timeEndPeriod(1);

                if (timeDiff >= 50.0 / 3.0 || g_GameWindow.usesRelativePath)
                {
                    while (timeDiff >= 50.0 / 3.0)
                    {
                        g_LastFrameTime += 50.0 / 3.0;
                        timeDiff -= 50.0 / 3.0;
                    }
                    if ((i32)g_Supervisor.cfg.frameskipConfig < (i32)this->curFrame)
                    {
                        goto LAB_00434a18;
                    }
                    goto begin_loop;
                }
            }
        }
    }

    if (!g_Supervisor.cfg.windowed && !g_Supervisor.VsyncEnabled())
    {
        if ((i32)g_Supervisor.cfg.frameskipConfig >= (i32)this->curFrame)
        {
            Present();
            goto begin_loop;
        }

    LAB_00434a18:
        Present();
        this->curFrame = 0;
        g_FrameCount++;
    }

    return RENDER_RESULT_KEEP_RUNNING;
}

i32 GameWindow::InitD3dInterface()
{
    g_Supervisor.d3dIface = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_Supervisor.d3dIface)
    {
        g_GameErrorContext.Fatal("Direct3D オブジェクトは何故か作成出来なかった\r\n");
        return true;
    }

    return false;
}

i32 GameWindow::CreateGameWindow(HINSTANCE hInstance)
{
    WNDCLASSA base_class;
    i32 width;
    i32 height;

    memset(&base_class, 0, sizeof(WNDCLASSA));
    base_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    base_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    base_class.hInstance = hInstance;
    base_class.lpfnWndProc = WindowProc;
    g_GameWindow.isAppActive = 1;
    g_GameWindow.isAppInactive = 0;
    base_class.lpszClassName = "BASE";
    RegisterClassA(&base_class);
    if (!g_Supervisor.cfg.windowed)
    {
        width = 640;
        height = 480;
        g_GameWindow.window =
            CreateWindowExA(0, "BASE", "東方妖々夢　〜 Perfect Cherry Blossom. ver 1.00b",
                            WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, NULL, hInstance, NULL);
    }
    else
    {
        width = GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + 640;
        height = 480 + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
        g_GameWindow.window =
            CreateWindowExA(0, "BASE", "東方妖々夢　〜 Perfect Cherry Blossom. ver 1.00b",
                            WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
                            width, height, NULL, NULL, hInstance, NULL);
    }
    g_Supervisor.hwndGameWindow = g_GameWindow.window;
    if (!g_GameWindow.window)
    {
        return true;
    }

    SetWindowActive(g_GameWindow.window);
    return false;
}

i32 GameWindow::InitD3dRendering()
{
    ZunVec3 pEye;
    ZunVec3 pAt;
    ZunVec3 pUp;
    char capsBuffer[8192];
    f32 fov;
    f32 aspectRatio;
    f32 halfWidth;
    f32 halfHeight;
    f32 halfCameraDistance;
    D3DPRESENT_PARAMETERS presentParams;
    D3DDISPLAYMODE displayMode;
    bool usingD3dHal;
    i32 retryWithoutRefreshRate;

    usingD3dHal = true;
    memset(&presentParams, 0, sizeof(D3DPRESENT_PARAMETERS));
    g_Supervisor.d3dIface->GetAdapterDisplayMode(0, &displayMode);
    if (!g_Supervisor.cfg.windowed)
    {
        if ((g_Supervisor.cfg.opts >> 2 & 1) == 1)
        {
            presentParams.BackBufferFormat = D3DFMT_R5G6B5;
            g_Supervisor.cfg.colorMode16bit = 1;
        }
        else if (g_Supervisor.cfg.colorMode16bit == 255)
        {
            presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
            g_Supervisor.cfg.colorMode16bit = 0;
            g_GameErrorContext.Log("初回起動、画面を 32Bits で初期化しました\r\n");
        }
        else if (!g_Supervisor.cfg.colorMode16bit)
        {
            presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
        }
        else
        {
            presentParams.BackBufferFormat = D3DFMT_R5G6B5;
        }
        if (g_GameWindow.usesRelativePath != false)
        {
            g_Supervisor.vsyncEnabled = 1;
        }
        if (!g_Supervisor.vsyncEnabled)
        {
            presentParams.FullScreen_RefreshRateInHz = 60;
            presentParams.FullScreen_PresentationInterval = 1;
            g_GameErrorContext.Log("リフレッシュレートを60Hzに変更を試みます\r\n");
            if (!g_Supervisor.cfg.frameskipConfig)
            {
                presentParams.SwapEffect = D3DSWAPEFFECT_FLIP;
            }
            else
            {
                presentParams.SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
            }
        }
        else
        {
            presentParams.FullScreen_RefreshRateInHz = 0;
            presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
            presentParams.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
            g_GameErrorContext.Log("VSync非同期可能かどうかを試みます\r\n");
        }
    }
    else
    {
        presentParams.BackBufferFormat = displayMode.Format;
        presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
        presentParams.Windowed = 1;
    }
    presentParams.BackBufferWidth = 640;
    presentParams.BackBufferHeight = 480;
    presentParams.EnableAutoDepthStencil = 1;
    presentParams.AutoDepthStencilFormat = D3DFMT_D16;
    presentParams.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    g_Supervisor.flags |= 2;
    g_Supervisor.lockableBackBuffer = 1;
    retryWithoutRefreshRate = 0;
    for (;;)
    {
        if ((g_Supervisor.cfg.opts >> 9 & 1) != 0)
        {
            goto fallback_to_software;
        }
        if (FAILED(g_Supervisor.d3dIface->CreateDevice(0, D3DDEVTYPE_HAL, g_GameWindow.window,
                                                       D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                                       &presentParams, &g_Supervisor.d3dDevice)))
        {
            if (retryWithoutRefreshRate)
            {
                g_GameErrorContext.Log("T&L HAL は使用できないようです\r\n");
            }
            if (FAILED(g_Supervisor.d3dIface->CreateDevice(
                    0, D3DDEVTYPE_HAL, g_GameWindow.window, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                    &presentParams, &g_Supervisor.d3dDevice)))
            {
                if (retryWithoutRefreshRate)
                {
                    g_GameErrorContext.Log("HAL も使用できないようです\r\n");
                }
            fallback_to_software:
                if (FAILED(g_Supervisor.d3dIface->CreateDevice(
                        0, D3DDEVTYPE_REF, g_GameWindow.window, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                        &presentParams, &g_Supervisor.d3dDevice)))
                {
                    if (!g_Supervisor.vsyncEnabled)
                    {
                        g_GameErrorContext.Log("リフレッシュレートが変更できません\r\n");
                        presentParams.FullScreen_RefreshRateInHz = 0;
                        g_Supervisor.lockableBackBuffer = 0;
                        retryWithoutRefreshRate = 1;
                        continue;
                    }

                    if (presentParams.FullScreen_PresentationInterval ==
                        D3DPRESENT_INTERVAL_IMMEDIATE)
                    {
                        g_GameErrorContext.Log(
                            "非同期更新も行えません。一番汚いモードに変更します\r\n");
                        g_GameErrorContext.Fatal(
                            "*** リフレッシュレートを60Hzに変更することを推奨します ***\r\n");
                        presentParams.FullScreen_PresentationInterval = 1;
                        presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
                        continue;
                    }
                    else
                    {
                        g_GameErrorContext.Fatal(
                            "Direct3D の初期化に失敗、これではゲームは出来ません\r\n");
                        SAFE_RELEASE(g_Supervisor.d3dIface);
                        return 1;
                    }
                }
                else
                {
                    g_GameErrorContext.Log(
                        "REF で動作しますが、重すぎて恐らくゲームになりません...\r\n");
                    g_Supervisor.flags &= 0xfffffffe;
                    usingD3dHal = false;
                }
            }
            else
            {
                g_GameErrorContext.Log("HAL で動作します\r\n");
                g_Supervisor.flags &= 0xfffffffe;
            }
        }
        else
        {
            g_GameErrorContext.Log("T&L HAL で動作しま〜す\r\n");
            g_Supervisor.flags |= 1;
        }
        break;
    }

    g_Supervisor.presentParameters = presentParams;
    halfWidth = 320.0f;
    halfHeight = 240.0f;
    aspectRatio = 1.3333334f;
    fov = 0.5235988f;
    halfCameraDistance = halfHeight / tanf(fov / 2.0f);
    pUp.x = 0.0f;
    pUp.y = 1.0f;
    pUp.z = 0.0f;
    pAt.x = halfWidth;
    pAt.y = -halfHeight;
    pAt.z = 0.0f;
    pEye.x = halfWidth;
    pEye.y = -halfHeight;
    pEye.z = -halfCameraDistance;
    g_Supervisor.viewMatrix.LookAtLH(&pEye, &pAt, &pUp);
    g_Supervisor.projectionMatrix.PerspectiveFovLH(fov, aspectRatio, 100.0f, 10000.0f);

    g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW, g_Supervisor.viewMatrix.asD3DX());
    g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION, g_Supervisor.projectionMatrix.asD3DX());
    g_Supervisor.d3dDevice->GetViewport(&g_Supervisor.viewport);
    g_Supervisor.d3dDevice->GetDeviceCaps(&g_Supervisor.d3dCaps);
    if ((g_Supervisor.cfg.opts & 1) == 0 && (g_Supervisor.d3dCaps.TextureOpCaps & 0x40) == 0)
    {
        g_GameErrorContext.Log(
            "D3DTEXOPCAPS_ADD をサポートしていません、色加算エミュレートモードで動作します\r\n");
        g_Supervisor.cfg.opts = g_Supervisor.cfg.opts | 1;
    }
    if (g_Supervisor.d3dCaps.MaxTextureWidth <= 256)
    {
        g_GameErrorContext.Log(
            "512 以上のテクスチャをサポートしていません。殆どの絵がボケて表示されます。\r\n");
    }
    FormatD3DCapabilities(&g_Supervisor.d3dCaps, capsBuffer);
    g_GameErrorContext.Log(capsBuffer);
    if ((g_Supervisor.cfg.opts >> 2 & 1) == 0 && usingD3dHal)
    {
        if (g_Supervisor.d3dIface->CheckDeviceFormat(0, D3DDEVTYPE_HAL,
                                                     presentParams.BackBufferFormat, 0,
                                                     D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) == 0)
        {
            g_Supervisor.flags |= 4;
        }
        else
        {
            g_Supervisor.flags &= 0xfffffffb;
            g_Supervisor.cfg.opts |= 4;
            g_GameErrorContext.Log(
                "D3DFMT_A8R8G8B8 をサポートしていません、減色モードで動作します\r\n");
        }
    }
    ResetRenderState();
    ScreenEffect::SetViewport(0xff000000);
    g_GameWindow.isAppClosing = 0;
    g_Supervisor.lastFrameTime = 0;
    return 0;
}

char *GameWindow::FormatCapability(const char *capabilityName, u32 capabilityFlags, u32 mask,
                                   char *buf)
{
    buf += sprintf(buf, "%s", capabilityName);
    if ((capabilityFlags & mask) == 0)
    {
        buf += sprintf(buf, "不可\r\n");
    }
    else
    {
        buf += sprintf(buf, "可\r\n");
    }
    return buf;
}

void GameWindow::FormatD3DCapabilities(D3DCAPS8 *caps, char *buf)
{
    char *strPos;

    strPos = buf;
    strPos += sprintf(strPos, "現在のビデオカード、及びドライバの能力詳細\r\n");
    strPos = FormatCapability("　走査線取得能力 : ", caps->Caps, D3DCAPS_READ_SCANLINE, strPos);
    strPos = FormatCapability("　ウィンドウモードのレンダリング : ", caps->Caps2,
                              D3DCAPS2_CANRENDERWINDOWED, strPos);
    strPos = FormatCapability("　プレゼンテーション間隔（直接）: ", caps->PresentationIntervals,
                              D3DPRESENT_INTERVAL_IMMEDIATE, strPos);
    strPos = FormatCapability("　プレゼンテーション間隔（垂直同期）: ", caps->PresentationIntervals,
                              D3DPRESENT_INTERVAL_ONE, strPos);
    strPos += sprintf(strPos, "　-- デバイス能力 ------------------------------\r\n");
    strPos = FormatCapability("　System -> 非ローカルVRAMブリット : ", caps->DevCaps,
                              D3DDEVCAPS_CANBLTSYSTONONLOCAL, strPos);
    strPos = FormatCapability("　ハードウェア T&L : ", caps->DevCaps,
                              D3DDEVCAPS_HWTRANSFORMANDLIGHT, strPos);
    strPos = FormatCapability("　非ローカルVRAMからテクスチャ取得 : ", caps->DevCaps,
                              D3DDEVCAPS_TEXTURENONLOCALVIDMEM, strPos);
    strPos = FormatCapability("　システムメモリからテクスチャ取得 : ", caps->DevCaps,
                              D3DDEVCAPS_TEXTURESYSTEMMEMORY, strPos);
    strPos = FormatCapability("　VRAM からテクスチャ取得 : ", caps->DevCaps,
                              D3DDEVCAPS_TEXTUREVIDEOMEMORY, strPos);
    strPos = FormatCapability("　頂点バッファにシステムメモリを使用 : ", caps->DevCaps,
                              D3DDEVCAPS_TLVERTEXSYSTEMMEMORY, strPos);
    strPos = FormatCapability("　頂点バッファにビデオメモリを使用 : ", caps->DevCaps,
                              D3DDEVCAPS_TLVERTEXVIDEOMEMORY, strPos);
    strPos += sprintf(strPos, "　-- プリミティブ能力 ---------------------------\r\n");
    strPos =
        FormatCapability("　半透明処理 : ", caps->PrimitiveMiscCaps, D3DPMISCCAPS_BLENDOP, strPos);
    strPos = FormatCapability("　ポイントのクリッピング処理 : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_CLIPPLANESCALEDPOINTS, strPos);
    strPos = FormatCapability("　プリミティブのクリッピング処理 : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_CLIPTLVERTS, strPos);
    strPos = FormatCapability("　法線クリップ（反時計周り） : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_CULLCCW, strPos);
    strPos = FormatCapability("　法線クリップ（時計周り） : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_CULLCW, strPos);
    strPos = FormatCapability("　法線クリップ無し : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_CULLNONE, strPos);
    strPos = FormatCapability("　デプステストON/OFF切り替え : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_MASKZ, strPos);
    strPos += sprintf(strPos, "　-- ラスタ能力 --------------------------------\r\n");
    strPos = FormatCapability("　異方性フィルタリング : ", caps->RasterCaps,
                              D3DPRASTERCAPS_ANISOTROPY, strPos);
    strPos = FormatCapability("　アンチエイリアシング : ", caps->RasterCaps,
                              D3DPRASTERCAPS_ANTIALIASEDGES, strPos);
    strPos = FormatCapability("　ディザ処理 : ", caps->RasterCaps, D3DPRASTERCAPS_DITHER, strPos);
    strPos = FormatCapability("　範囲ベースのフォグ : ", caps->RasterCaps, D3DPRASTERCAPS_FOGRANGE,
                              strPos);
    strPos =
        FormatCapability("　Zベースのフォグ : ", caps->RasterCaps, D3DPRASTERCAPS_ZFOG, strPos);
    strPos =
        FormatCapability("　テーブルフォグ : ", caps->RasterCaps, D3DPRASTERCAPS_FOGTABLE, strPos);
    strPos =
        FormatCapability("　頂点フォグ : ", caps->RasterCaps, D3DPRASTERCAPS_FOGVERTEX, strPos);
    strPos = FormatCapability("　デプステスト : ", caps->RasterCaps, D3DPRASTERCAPS_ZTEST, strPos);
    strPos += sprintf(strPos, "　-- シェーディング能力 -----------------------\r\n");
    strPos = FormatCapability("　グーローシェーディング : ", caps->ShadeCaps,
                              D3DPSHADECAPS_COLORGOURAUDRGB, strPos);
    strPos = FormatCapability("　α成分のグーローシェーディング : ", caps->ShadeCaps,
                              D3DPSHADECAPS_ALPHAGOURAUDBLEND, strPos);
    strPos = FormatCapability("　グーローシェーディングでフォグ : ", caps->ShadeCaps,
                              D3DPSHADECAPS_FOGGOURAUD, strPos);
    strPos += sprintf(strPos, "　-- テクスチャ能力 ---------------------------\r\n");
    strPos += sprintf(strPos, "　最大テクスチャサイズ : (%lu, %lu)\r\n", caps->MaxTextureWidth,
                      caps->MaxTextureHeight);
    strPos =
        FormatCapability("　α付きテクスチャ : ", caps->TextureCaps, D3DPTEXTURECAPS_ALPHA, strPos);
    strPos = FormatCapability("　テクスチャトランスフォーム : ", caps->TextureCaps,
                              D3DPTEXTURECAPS_PROJECTED, strPos);
    strPos = FormatCapability("　バイリニア補間（拡大） : ", caps->TextureFilterCaps,
                              D3DPTFILTERCAPS_MAGFLINEAR, strPos);
    strPos = FormatCapability("　バイリニア補間（縮小） : ", caps->TextureFilterCaps,
                              D3DPTFILTERCAPS_MINFLINEAR, strPos);
    strPos += sprintf(strPos, "--------------------------------------------\r\n");
}

void GameWindow::ResetRenderState()
{
    if ((g_Supervisor.cfg.opts >> 6 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZENABLE, 1);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_LIGHTING, 0);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_CULLMODE, 1);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    if ((g_Supervisor.cfg.opts >> 5 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_SHADEMODE, 2);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_SHADEMODE, 1);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_SRCBLEND, 5);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZFUNC, 8);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHAREF, 4);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHAFUNC, 7);
    if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGENABLE, 1);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGENABLE, 0);
    }
    f32 fogDensity = 1.0f;
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGDENSITY, *(u32 *)&fogDensity);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, 0);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGVERTEXMODE, 3);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGCOLOR, 0xffa0a0a0);

    f32 fog = 1000.0f;
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGSTART, *(u32 *)&fog);
    fog = 5000.0f;
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGEND, *(u32 *)&fog);
    if ((g_Supervisor.d3dCaps.RasterCaps | 0x1000) != 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_EDGEANTIALIAS, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 0);
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
    if ((g_Supervisor.cfg.opts >> 1 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 3);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 0);
    }
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
    if ((g_Supervisor.cfg.opts >> 1 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 3);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 0);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, 3);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, 1);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, 1);
    if (g_AnmManager)
    {
        g_AnmManager->SetBlendMode(255);
        g_AnmManager->SetColorOp(255);
        g_AnmManager->SetVertexShader(255);
        g_AnmManager->SetTexture(NULL);
        g_AnmManager->SetCameraMode(255);
    }
    g_Stage.renderStateWasReset = 1;
}

ZunResult GameWindow::CheckForRunningGameInstance(HINSTANCE hInstance)
{
    char *ext;
    char resolvedPath[264];
    STARTUPINFO startupInfo;
    char exePath[264];
    g_Mutex = CreateMutexA(NULL, 1, "Touhou YouYouMu App");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        g_GameErrorContext.Fatal("二つは起動できません\r\n");
        return ZUN_ERROR;
    }

    startupInfo.cb = sizeof(startupInfo);
    memset(&startupInfo.lpReserved, 0, sizeof(startupInfo) - 4);
    GetModuleFileNameA(NULL, exePath, 0x105);
    GetConsoleTitleA(resolvedPath, 0x105);
    GetStartupInfoA(&startupInfo);
    if (startupInfo.lpTitle)
    {
        ext = strrchr(startupInfo.lpTitle, '.');
        if (FileSystem::CheckFileExists(startupInfo.lpTitle) && ext)
        {
            if (_stricmp(ext, ".lnk") == 0)
            {
                do
                {
                    ResolveIt(startupInfo.lpTitle, resolvedPath, 0x104);
                    ext = strrchr(resolvedPath, '.');
                } while (_stricmp(ext, ".lnk") == 0);
            }
            else
            {
                strcpy(resolvedPath, startupInfo.lpTitle);
            }

            if (strcmp(exePath, resolvedPath) != 0)
            {
                g_GameWindow.usesRelativePath = true;
            }
        }
    }
    if (!g_Mutex)
    {
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

void GameWindow::SetWindowActive(HWND window)
{
    u32 idAttachTo;
    void *param;
    u32 processId;

    idAttachTo = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    processId = GetWindowThreadProcessId(window, NULL);
    AttachThreadInput(processId, idAttachTo, 1);
    SystemParametersInfoA(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &param, 0);
    SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);

    SetForegroundWindow(window);
    SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &param, 0);
    AttachThreadInput(processId, idAttachTo, 0);
}

i32 GameWindow::ChecksumExecutable()
{
    u32 *dataBase;
    u32 checksum;
    u32 *dataCursor;
    u32 i;
    char filename[MAX_PATH + 1];

    if (GetModuleFileNameA(NULL, filename, 0x105))
    {
        checksum = 0;
        dataBase = dataCursor = (u32 *)FileSystem::OpenFile(filename, 1);
        if (!dataCursor)
        {
            return -1;
        }

        for (i = 0; i < g_LastFileSize / 4 - 1; i++, dataCursor++)
        {
            checksum += *dataCursor;
        }
        Supervisor::DebugPrint("main sum %d\r\n", checksum);
        free(dataBase);
        g_Supervisor.exeChecksum = checksum;
        g_Supervisor.exeSize = g_LastFileSize;
        return checksum;
    }

    return -1;
}

i32 GameWindow::ResolveIt(const char *shortcutPath, char *dstPath, i32 maxPathLen)
{
    WIN32_FIND_DATAA wfd;
    LPWSTR wPath;
    IPersistFile *ppf;
    IShellLinkA *psl;
    i32 ret;
    HRESULT hr;

    if (!dstPath)
    {
        return 0;
    }

    ret = 0;
    CoInitialize(NULL);
    if (SUCCEEDED(hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink,
                                        (void **)&psl)))
    {
        if (SUCCEEDED(hr = psl->QueryInterface(IID_IPersistFile, (void **)&ppf)))
        {
            wPath = new WCHAR[maxPathLen];
            if (SUCCEEDED(hr))
            {
                MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wPath, maxPathLen);
                if (SUCCEEDED(hr = ppf->Load(wPath, STGM_READ)))
                {
                    if (SUCCEEDED(hr = psl->GetPath(dstPath, maxPathLen, &wfd, 0)))
                    {
                        ret = 1;
                    }
                }
            }
            delete wPath;
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return ret;
}
