#pragma once

#include "AnmManager.hpp"
#include "AnmVm.hpp"
#include "ScreenEffect.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "utils.hpp"

struct StageAnms
{
    const char *anmPath1;
    const char *anmPath2;
};
extern StageAnms g_EnemyAnmStageFiles[9];

struct StdRawHeader
{
    i16 objectsCount;
    i16 quadCount;
    u32 facesOffset;
    u32 scriptOffset;
    u32 unused_c;
    char stageName[128];
    char bgmNames[4][128];
    char bgmPaths[4][128];
};

struct StdRawQuadBasic
{
    i16 type;
    i16 byteSize;
    i16 anmScript;
    i16 vmIndex;
    D3DXVECTOR3 pos;
    D3DXVECTOR2 size;
};

struct StdRawObject
{
    u16 id;
    i8 zLevel;
    i8 flags;
    D3DXVECTOR3 pos;
    D3DXVECTOR3 size;
    StdRawQuadBasic firstQuad;
};

struct StdRawInstance
{
    i16 id;
    i16 field1_0x2;
    D3DXVECTOR3 pos;
};

struct StdRawInstr
{
    i32 frame;
    i16 opcode;
    i16 size;
    AnyArg args[3];
};

struct StageCameraSky
{
    f32 nearPlane;
    f32 farPlane;
};

struct StageCamera
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 lookAt;
    D3DXVECTOR3 up;
    D3DXVECTOR3 lookAtDir;
    D3DXVECTOR3 right;
    f32 fov;
};

struct Stage
{
    Stage();

    static ZunResult RegisterChain(i32 stage);
    static void CutChain();

    static ZunResult AddedCallback(Stage *arg);
    static ZunResult DeletedCallback(Stage *arg);
    static u32 OnUpdate(Stage *arg);
    static u32 OnDrawHighPrio(Stage *arg);
    static u32 OnDrawLowPrio(Stage *arg);

    static void DrawColoredQuad(ZunRect *rect, D3DCOLOR param_2, D3DCOLOR param_3,
                                D3DCOLOR param_4, D3DCOLOR param_5);
    ZunResult LoadStageData(const char *stdPath);
    i32 RenderObjects(i32 param_1);
    void SmoothBlendColor(ZunColor param_1);
    void UpdateCamera();
    ZunResult UpdateObjects();
    void SetupCameraStageBackground();
    static void UpdateScriptAndCamera(Stage *stage, i32 param_2,
                                      D3DXVECTOR3 *param_3, D3DXVECTOR3 *param_4,
                                      D3DXVECTOR3 *param_5, D3DXVECTOR3 *param_6,
                                      D3DXVECTOR3 *param_7);

    static u8 ClampColorChannel(u32 src)
    {
        if (src >= 256)
        {
            src = 255;
        }
        return src;
    }

    AnmVm *quadVms;
    AnmVm vm1;
    AnmVm vm2;
    StdRawHeader *stdData;
    i32 quadCount;
    i32 objectsCount;
    StdRawObject **objects;
    StdRawInstance *objectInstances;
    StdRawInstr *beginningOfScript;
    ZunTimer scriptTime;
    i32 instructionIndex;
    i32 stageFrameCounter;
    u32 stage;
    D3DXVECTOR3 position;
    D3DCOLOR color;
    StageCameraSky skyFog;
    ZunColor fogColor;
    f32 fogNearPlaneEnd;
    f32 fogFarPlaneEnd;
    ZunColor fogColorEnd;
    f32 fogNearPlaneStart;
    f32 fogFarPlaneStart;
    ZunColor fogColorStart;
    i32 skyFogInterpDuration;
    ZunTimer skyFogInterpTimer;
    u8 renderStateWasReset;
    // pad 3
    i32 spellCardState;
    i32 ticksSinceSpellcardStarted;
    i32 clearBackground;
    i32 numSpellcardVms;
    i32 spellcardVmsIdx;
    AnmVm spellcardVms[33];
    i32 scriptWaitTime;
    StageCamera camEnd;
    StageCamera camStart;
    StageCamera camTangentEnd;
    StageCamera camTangentStart;
    StageCamera cam;
    i32 timersMax[4];
    ZunTimer timers[4];
    i32 interpModes[4];
    D3DXVECTOR3 positionStart;
    i32 positionInterpEndTime;
    D3DXVECTOR3 positionInterpInitial;
    i32 positionInterpStartTime;
    u8 cameraTeleported;
    // pad 3
    ZunColor color2;
    i32 isDarkening;
};
C_ASSERT(sizeof(Stage) == 0x52b4);
extern Stage g_Stage;
