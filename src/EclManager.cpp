#include "EclManager.hpp"

#include <stdio.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "EnemyEclInstr.hpp"
#include "EnemyManager.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"
#include "dsutil.hpp"

#define GET_INT_PTR(enemy, argIdx) \
    GetVar(enemy, &instr->args[argIdx].i, instr->paramMask, argIdx)

#define GET_FLOAT_PTR(enemy, argIdx) \
    GetFloatVar(enemy, &instr->args[argIdx].f, instr->paramMask, argIdx)

#define GET_INT_VALUE(enemy, argIdx) \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetVarValue(enemy, instr->args[argIdx].i) : instr->args[argIdx].i)

#define GET_FLOAT_VALUE(enemy, argIdx) \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetFloatVarValue(enemy, instr->args[argIdx].f) : instr->args[argIdx].f)

#define GET_INT_VALUE_D(enemy, argIdx, bitIdx) \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetVarValue(enemy, instr->args[argIdx].i) : instr->args[argIdx].i)

#define GET_FLOAT_VALUE_D(enemy, argIdx, bitIdx) \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetFloatVarValue(enemy, instr->args[argIdx].f) : instr->args[argIdx].f)

// GLOBAL: TH07 0x0049f560
const char *g_EclPaths[10] = {
    "dummy",
    // STRING: TH07 0x00497f38
    "data/ecldata1.ecl",
    // STRING: TH07 0x00497f24
    "data/ecldata2.ecl",
    // STRING: TH07 0x00497f10
    "data/ecldata3.ecl",
    // STRING: TH07 0x00497efc
    "data/ecldata4.ecl",
    // STRING: TH07 0x00497ee8
    "data/ecldata5.ecl",
    // STRING: TH07 0x00497ed4
    "data/ecldata6.ecl",
    // STRING: TH07 0x00497ec0
    "data/ecldata7.ecl",
    // STRING: TH07 0x00497eac
    "data/ecldata8.ecl",
    NULL,
};

// GLOBAL: TH07 0x01347938
EclManager g_EclManager;

// GLOBAL: TH07 0x01347aa0
EclGlobalVars g_GlobalEclVars;

// FUNCTION: TH07 0x0040e420
ZunResult EclManager::Load(const char *path)
{
    i32 i;

    this->eclFile = (EclRawHeader *)FileSystem::OpenFile(path, 0);
    if (this->eclFile == NULL)
    {
        // STRING: TH07 0x00498700
        g_GameErrorContext.Log("敵データの読み込みに失敗しました、データが壊れてるか失われています\r\n");
        return ZUN_ERROR;
    }

    for (i = 0; i < 0x10; i++)
    {
        this->eclFile->timelinePtr[i] =
            (EclTimelineInstr *)((i32)this->eclFile->timelinePtr[i] + (i32)this->eclFile);
    }
    this->subTable = this->eclFile->subTable;
    for (i = 0; i < this->eclFile->subCount; i++)
    {
        this->subTable[i] =
            (EclRawInstr *)((i32)this->subTable[i] + (i32)this->eclFile);
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0040e4f0
void EclManager::Unload()
{
    if (this->eclFile != NULL)
    {
        ZunMemory::Free(this->eclFile);
    }
    this->eclFile = NULL;
}

// FUNCTION: TH07 0x0040e530
ZunResult EclManager::CallEclSub(EnemyEclContext *param_1, i16 subId)
{
    param_1->curInstr = this->subTable[subId];
    param_1->time = 0;
    param_1->timer2 = 0;
    param_1->subId = subId;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0040e5b0
i32 EclManager::GetVarValue(Enemy *enemy, i32 eclVar)
{
    switch (eclVar)
    {
    case VAR_LOCAL_INT1_1:
        return enemy->currentContext.eclContextArgs.intVars1[0];
    case VAR_LOCAL_INT1_2:
        return enemy->currentContext.eclContextArgs.intVars1[1];
    case VAR_LOCAL_INT1_3:
        return enemy->currentContext.eclContextArgs.intVars1[2];
    case VAR_LOCAL_INT1_4:
        return enemy->currentContext.eclContextArgs.intVars1[3];
    case VAR_LOCAL_INT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case VAR_LOCAL_INT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LOCAL_INT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case VAR_LOCAL_INT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case VAR_LOCAL_INT2_1:
        return enemy->currentContext.eclContextArgs.intVars2[0];
    case VAR_LOCAL_INT2_2:
        return enemy->currentContext.eclContextArgs.intVars2[1];
    case VAR_LOCAL_INT2_3:
        return enemy->currentContext.eclContextArgs.intVars2[2];
    case VAR_LOCAL_INT2_4:
        return enemy->currentContext.eclContextArgs.intVars2[3];
    case VAR_DIFFICULTY:
        return g_GameManager.difficulty;
    case VAR_RANK:
        return g_GameManager.rank.rank;
    case VAR_CUR_TIME:
        return enemy->timer.current;
    case VAR_LIFE:
        return enemy->life;
    case VAR_PLAYER_SHOTTYPE:
        return g_GameManager.shotTypeAndCharacter;
    case VAR_LOCAL_FLOAT2_1:
        return enemy->currentContext.eclContextArgs.floatVars2[0];
    case VAR_LOCAL_FLOAT2_2:
        return enemy->currentContext.eclContextArgs.floatVars2[1];
    case VAR_LOCAL_FLOAT1_1:
        return enemy->currentContext.eclContextArgs.floatVars1[0];
    case VAR_LOCAL_FLOAT1_2:
        return enemy->currentContext.eclContextArgs.floatVars1[1];
    case VAR_LOCAL_FLOAT1_3:
        return enemy->currentContext.eclContextArgs.floatVars1[2];
    case VAR_LOCAL_FLOAT1_4:
        return enemy->currentContext.eclContextArgs.floatVars1[3];
    case VAR_LOCAL_FLOAT1_5:
        return enemy->currentContext.eclContextArgs.floatVars1[4];
    case VAR_LOCAL_FLOAT1_6:
        return enemy->currentContext.eclContextArgs.floatVars1[5];
    case VAR_LOCAL_FLOAT1_7:
        return enemy->currentContext.eclContextArgs.floatVars1[6];
    case VAR_LOCAL_FLOAT1_8:
        return enemy->currentContext.eclContextArgs.floatVars1[7];
    case VAR_LOCAL_FLOAT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case VAR_LOCAL_FLOAT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_LOCAL_FLOAT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case VAR_LOCAL_FLOAT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case VAR_GLOBAL_INT_1:
        return g_GlobalEclVars.intVars[0];
    case VAR_GLOBAL_INT_2:
        return g_GlobalEclVars.intVars[1];
    case VAR_GLOBAL_INT_3:
        return g_GlobalEclVars.intVars[2];
    case VAR_GLOBAL_INT_4:
        return g_GlobalEclVars.intVars[3];
    case VAR_GLOBAL_FLOAT_1:
        return g_GlobalEclVars.floatVars[0];
    case VAR_GLOBAL_FLOAT_2:
        return g_GlobalEclVars.floatVars[1];
    case VAR_GLOBAL_FLOAT_3:
        return g_GlobalEclVars.floatVars[2];
    case VAR_GLOBAL_FLOAT_4:
        return g_GlobalEclVars.floatVars[3];
    case VAR_POS_X:
        return enemy->position.x;
    case VAR_POS_Y:
        return enemy->position.y;
    case VAR_POS_Z:
        return enemy->position.z;
    case VAR_PLAYER_POS_X:
        return g_Player.positionCenter.x;
    case VAR_PLAYER_POS_Y:
        return g_Player.positionCenter.y;
    case VAR_PLAYER_POS_Z:
        return g_Player.positionCenter.z;
    case VAR_MOVE_INTERP_ORIGIN_X:
        return enemy->moveInterpStartPos.x;
    case VAR_MOVE_INTERP_ORIGIN_Y:
        return enemy->moveInterpStartPos.y;
    case VAR_MOVE_INTERP_ORIGIN_Z:
        return enemy->moveInterpStartPos.z;
    case VAR_DELTA_POS_X:
        return enemy->deltaPos.x;
    case VAR_DELTA_POS_Y:
        return enemy->deltaPos.y;
    case VAR_DELTA_POS_Z:
        return enemy->deltaPos.z;
    case VAR_BOSS_LIFE_THRESHOLD1:
        return enemy->lifeCallbackThreshold[0];
    case VAR_BOSS_LIFE_THRESHOLD2:
        return enemy->lifeCallbackThreshold[1];
    case VAR_BOSS_LIFE_THRESHOLD3:
        return enemy->lifeCallbackThreshold[2];
    case VAR_BOSS_LIFE_THRESHOLD4:
        return enemy->lifeCallbackThreshold[3];
    case VAR_ANGLE:
        return enemy->angle;
    case VAR_ANGULAR_VELOCITY:
        return enemy->angularVelocity;
    case VAR_MOVE_SPEED:
        return enemy->moveSpeed;
    case VAR_MOVE_ACCELERATION:
        return enemy->moveAcceleration;
    case VAR_MOVE_RADIUS:
        return enemy->moveRadius;
    case VAR_MOVE_ANGLE:
        return enemy->moveAngle;
    case VAR_MOVE_ANGULAR_VELOCITY:
        return enemy->moveAngularVelocity;
    case VAR_RNG:
        return g_Rng.GetRandomU32();
    case VAR_RNG_CUSTOM_BOUND:
        return g_Rng.GetRandomU32InRange(
                   enemy->currentContext.eclContextArgs.globalVars.intVars[0]) +
               enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LAST_DAMAGE:
        return enemy->lastDamage;
    case VAR_BOSS_ID:
        return enemy->bossId;
    case VAR_ITEMDROP:
        return enemy->itemDrop;
    case VAR_SCORE:
        return enemy->score;
    case VAR_ANGLE_TO_PLAYER:
        return g_Player.AngleToPlayer(&enemy->position);
    case VAR_DISTANCE_FROM_PLAYER:
        return D3DXVec3Length(&(g_Player.positionCenter - enemy->position));
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040ec00
i32 *EclManager::GetVar(Enemy *enemy, i32 *eclVar, u16 paramMask, i32 param_4)
{
    if ((param_4 >= 0) && (((u32)paramMask & 1 << param_4) == 0))
    {
        return eclVar;
    }

    switch (*eclVar)
    {
    case VAR_LOCAL_INT1_1:
        return &enemy->currentContext.eclContextArgs.intVars1[0];
    case VAR_LOCAL_INT1_2:
        return &enemy->currentContext.eclContextArgs.intVars1[1];
    case VAR_LOCAL_INT1_3:
        return &enemy->currentContext.eclContextArgs.intVars1[2];
    case VAR_LOCAL_INT1_4:
        return &enemy->currentContext.eclContextArgs.intVars1[3];
    case VAR_LOCAL_INT3_1:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case VAR_LOCAL_INT3_2:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LOCAL_INT3_3:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case VAR_LOCAL_INT3_4:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case VAR_LOCAL_INT2_1:
        return &enemy->currentContext.eclContextArgs.intVars2[0];
    case VAR_LOCAL_INT2_2:
        return &enemy->currentContext.eclContextArgs.intVars2[1];
    case VAR_LOCAL_INT2_3:
        return &enemy->currentContext.eclContextArgs.intVars2[2];
    case VAR_LOCAL_INT2_4:
        return &enemy->currentContext.eclContextArgs.intVars2[3];
    case VAR_DIFFICULTY:
        return &g_GameManager.difficulty;
    case VAR_RANK:
        return &g_GameManager.rank.rank;
    case VAR_CUR_TIME:
        return &enemy->timer.current;
    case VAR_LIFE:
        return &enemy->life;
    case VAR_ITEMDROP:
        return &enemy->itemDrop;
    case VAR_SCORE:
        return &enemy->score;
    case VAR_GLOBAL_INT_1:
        return &g_GlobalEclVars.intVars[0];
    case VAR_GLOBAL_INT_2:
        return &g_GlobalEclVars.intVars[1];
    case VAR_GLOBAL_INT_3:
        return &g_GlobalEclVars.intVars[2];
    case VAR_GLOBAL_INT_4:
        return &g_GlobalEclVars.intVars[3];
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040edf0
f32 EclManager::GetFloatVarValue(Enemy *enemy, f32 eclVar)
{
    switch ((i32)eclVar)
    {
    case VAR_LOCAL_INT1_1:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[0];
    case VAR_LOCAL_INT1_2:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[1];
    case VAR_LOCAL_INT1_3:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[2];
    case VAR_LOCAL_INT1_4:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[3];
    case VAR_LOCAL_INT3_1:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case VAR_LOCAL_INT3_2:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LOCAL_INT3_3:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case VAR_LOCAL_INT3_4:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case VAR_LOCAL_INT2_1:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[0];
    case VAR_LOCAL_INT2_2:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[1];
    case VAR_LOCAL_INT2_3:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[2];
    case VAR_LOCAL_INT2_4:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[3];
    case VAR_DIFFICULTY:
        return (f32)g_GameManager.difficulty;
    case VAR_RANK:
        return (f32)g_GameManager.rank.rank;
    case VAR_CUR_TIME:
        return (f32)enemy->timer.current;
    case VAR_LIFE:
        return (f32)enemy->life;
    case VAR_PLAYER_SHOTTYPE:
        return (f32)g_GameManager.shotTypeAndCharacter;
    case VAR_ITEMDROP:
        return (f32)enemy->itemDrop;
    case VAR_SCORE:
        return (f32)enemy->score;
    case VAR_GLOBAL_INT_1:
        return (f32)g_GlobalEclVars.intVars[0];
    case VAR_GLOBAL_INT_2:
        return (f32)g_GlobalEclVars.intVars[1];
    case VAR_GLOBAL_INT_3:
        return (f32)g_GlobalEclVars.intVars[2];
    case VAR_GLOBAL_INT_4:
        return (f32)g_GlobalEclVars.intVars[3];
    case VAR_GLOBAL_FLOAT_1:
        return g_GlobalEclVars.floatVars[0];
    case VAR_GLOBAL_FLOAT_2:
        return g_GlobalEclVars.floatVars[1];
    case VAR_GLOBAL_FLOAT_3:
        return g_GlobalEclVars.floatVars[2];
    case VAR_GLOBAL_FLOAT_4:
        return g_GlobalEclVars.floatVars[3];
    case VAR_LOCAL_FLOAT1_1:
        return enemy->currentContext.eclContextArgs.floatVars1[0];
    case VAR_LOCAL_FLOAT1_2:
        return enemy->currentContext.eclContextArgs.floatVars1[1];
    case VAR_LOCAL_FLOAT1_3:
        return enemy->currentContext.eclContextArgs.floatVars1[2];
    case VAR_LOCAL_FLOAT1_4:
        return enemy->currentContext.eclContextArgs.floatVars1[3];
    case VAR_LOCAL_FLOAT1_5:
        return enemy->currentContext.eclContextArgs.floatVars1[4];
    case VAR_LOCAL_FLOAT1_6:
        return enemy->currentContext.eclContextArgs.floatVars1[5];
    case VAR_LOCAL_FLOAT1_7:
        return enemy->currentContext.eclContextArgs.floatVars1[6];
    case VAR_LOCAL_FLOAT1_8:
        return enemy->currentContext.eclContextArgs.floatVars1[7];
    case VAR_LOCAL_FLOAT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case VAR_LOCAL_FLOAT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_LOCAL_FLOAT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case VAR_LOCAL_FLOAT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case VAR_POS_X:
        return enemy->position.x;
    case VAR_POS_Y:
        return enemy->position.y;
    case VAR_POS_Z:
        return enemy->position.z;
    case VAR_PLAYER_POS_X:
        return g_Player.positionCenter.x;
    case VAR_PLAYER_POS_Y:
        return g_Player.positionCenter.y;
    case VAR_PLAYER_POS_Z:
        return g_Player.positionCenter.z;
    case VAR_LOCAL_FLOAT2_1:
        return enemy->currentContext.eclContextArgs.floatVars2[0];
    case VAR_LOCAL_FLOAT2_2:
        return enemy->currentContext.eclContextArgs.floatVars2[1];
    case VAR_MOVE_INTERP_ORIGIN_X:
        return enemy->moveInterpStartPos.x;
    case VAR_MOVE_INTERP_ORIGIN_Y:
        return enemy->moveInterpStartPos.y;
    case VAR_MOVE_INTERP_ORIGIN_Z:
        return enemy->moveInterpStartPos.z;
    case VAR_MOVE_INTERP_TARGET_X:
        return enemy->moveInterp.x;
    case VAR_MOVE_INTERP_TARGET_Y:
        return enemy->moveInterp.y;
    case VAR_MOVE_INTERP_TARGET_Z:
        return enemy->moveInterp.z;
    case VAR_DELTA_POS_X:
        return enemy->deltaPos.x;
    case VAR_DELTA_POS_Y:
        return enemy->deltaPos.y;
    case VAR_DELTA_POS_Z:
        return enemy->deltaPos.z;
    case VAR_BOSS_LIFE_THRESHOLD1:
        return (f32)enemy->lifeCallbackThreshold[0];
    case VAR_BOSS_LIFE_THRESHOLD2:
        return (f32)enemy->lifeCallbackThreshold[1];
    case VAR_BOSS_LIFE_THRESHOLD3:
        return (f32)enemy->lifeCallbackThreshold[2];
    case VAR_BOSS_LIFE_THRESHOLD4:
        return (f32)enemy->lifeCallbackThreshold[3];
    case VAR_ANGLE_TO_PLAYER:
        return g_Player.AngleToPlayer(&enemy->position);
    case VAR_ANGLE:
        return enemy->angle;
    case VAR_ANGULAR_VELOCITY:
        return enemy->angularVelocity;
    case VAR_MOVE_SPEED:
        return enemy->moveSpeed;
    case VAR_MOVE_ACCELERATION:
        return enemy->moveAcceleration;
    case VAR_MOVE_RADIUS:
        return enemy->moveRadius;
    case VAR_MOVE_ANGLE:
        return enemy->moveAngle;
    case VAR_MOVE_ANGULAR_VELOCITY:
        return enemy->moveAngularVelocity;
    case VAR_RNG:
        return g_Rng.GetRandomFloat();
    case VAR_RNG_CUSTOM_BOUND:
        return g_Rng.GetRandomFloatInRange(enemy->currentContext.eclContextArgs.globalVars.floatVars[0]) +
               enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_RNG_RADIAN:
        return g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    case VAR_BOSS_ID:
        return (f32)enemy->bossId;
    case VAR_LAST_DAMAGE:
        return (f32)enemy->lastDamage;
    case VAR_DISTANCE_FROM_PLAYER:
        return D3DXVec3Length(&(g_Player.positionCenter - enemy->position));
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040f3c0
f32 *EclManager::GetFloatVar(Enemy *enemy, f32 *eclVar, u16 paramMask,
                             i32 param_4)
{
    if ((param_4 >= 0) && (((u32)paramMask & 1 << param_4) == 0))
    {
        return eclVar;
    }

    switch ((i32)*eclVar)
    {
    case VAR_LOCAL_FLOAT1_1:
        return &enemy->currentContext.eclContextArgs.floatVars1[0];
    case VAR_LOCAL_FLOAT1_2:
        return &enemy->currentContext.eclContextArgs.floatVars1[1];
    case VAR_LOCAL_FLOAT1_3:
        return &enemy->currentContext.eclContextArgs.floatVars1[2];
    case VAR_LOCAL_FLOAT1_4:
        return &enemy->currentContext.eclContextArgs.floatVars1[3];
    case VAR_LOCAL_FLOAT1_5:
        return &enemy->currentContext.eclContextArgs.floatVars1[4];
    case VAR_LOCAL_FLOAT1_6:
        return &enemy->currentContext.eclContextArgs.floatVars1[5];
    case VAR_LOCAL_FLOAT1_7:
        return &enemy->currentContext.eclContextArgs.floatVars1[6];
    case VAR_LOCAL_FLOAT1_8:
        return &enemy->currentContext.eclContextArgs.floatVars1[7];
    case VAR_LOCAL_FLOAT3_1:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case VAR_LOCAL_FLOAT3_2:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_LOCAL_FLOAT3_3:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case VAR_LOCAL_FLOAT3_4:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case VAR_POS_X:
        return &enemy->position.x;
    case VAR_POS_Y:
        return &enemy->position.y;
    case VAR_POS_Z:
        return &enemy->position.z;
    case VAR_PLAYER_POS_X:
        return &g_Player.positionCenter.x;
    case VAR_PLAYER_POS_Y:
        return &g_Player.positionCenter.y;
    case VAR_PLAYER_POS_Z:
        return &g_Player.positionCenter.z;
    case VAR_LOCAL_FLOAT2_1:
        return &enemy->currentContext.eclContextArgs.floatVars2[0];
    case VAR_LOCAL_FLOAT2_2:
        return &enemy->currentContext.eclContextArgs.floatVars2[1];
    case VAR_GLOBAL_FLOAT_1:
        return &g_GlobalEclVars.floatVars[0];
    case VAR_GLOBAL_FLOAT_2:
        return &g_GlobalEclVars.floatVars[1];
    case VAR_GLOBAL_FLOAT_3:
        return &g_GlobalEclVars.floatVars[2];
    case VAR_GLOBAL_FLOAT_4:
        return &g_GlobalEclVars.floatVars[3];
    case VAR_MOVE_INTERP_ORIGIN_X:
        return &enemy->moveInterpStartPos.x;
    case VAR_MOVE_INTERP_ORIGIN_Y:
        return &enemy->moveInterpStartPos.y;
    case VAR_MOVE_INTERP_ORIGIN_Z:
        return &enemy->moveInterpStartPos.z;
    case VAR_MOVE_INTERP_TARGET_X:
        return &enemy->moveInterp.x;
    case VAR_MOVE_INTERP_TARGET_Y:
        return &enemy->moveInterp.y;
    case VAR_MOVE_INTERP_TARGET_Z:
        return &enemy->moveInterp.z;
    case VAR_ANGLE:
        return &enemy->angle;
    case VAR_ANGULAR_VELOCITY:
        return &enemy->angularVelocity;
    case VAR_MOVE_SPEED:
        return &enemy->moveSpeed;
    case VAR_MOVE_ACCELERATION:
        return &enemy->moveAcceleration;
    case VAR_MOVE_RADIUS:
        return &enemy->moveRadius;
    case VAR_MOVE_ANGLE:
        return &enemy->moveAngle;
    case VAR_MOVE_ANGULAR_VELOCITY:
        return &enemy->moveAngularVelocity;
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040f6b0
void EclManager::MoveDirTime(Enemy *enemy, EclRawInstr *instr)
{
    f32 fVar2;

    fVar2 = utils::AddNormalizeAngle(GET_FLOAT_VALUE(enemy, 2), 0.0f);
    enemy->moveInterp.x = cosf(fVar2) * GET_FLOAT_VALUE(enemy, 3) *
                          (f32)GET_INT_VALUE(enemy, 0);
    enemy->moveInterp.y = sinf(fVar2) * GET_FLOAT_VALUE(enemy, 3) *
                          (f32)GET_INT_VALUE(enemy, 0);
    enemy->moveInterp.z = 0.0f;
    enemy->moveInterpStartPos = enemy->position;
    enemy->moveInterpTimer = enemy->moveInterpStartTime =
        GET_INT_VALUE(enemy, 0);
    enemy->interpEasing = (u8)GET_INT_VALUE(enemy, 1);
    enemy->moveMode = 2;
    if (enemy->mirror)
    {
        enemy->moveInterp.x = -enemy->moveInterp.x;
    }
}

// FUNCTION: TH07 0x0040f8f0
void EclManager::MovePosTime(Enemy *enemy, EclRawInstr *instr)
{
    D3DXVECTOR3 newPos;
    newPos.x = GET_FLOAT_VALUE(enemy, 2);
    newPos.y = GET_FLOAT_VALUE(enemy, 3);
    newPos.z = GET_FLOAT_VALUE(enemy, 4);

    enemy->moveInterp = newPos - enemy->position;
    enemy->moveInterpStartPos = enemy->position;
    enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
    enemy->interpEasing = (u8)GET_INT_VALUE(enemy, 1);
    enemy->moveMode = 2;
    enemy->axisSpeed = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    if (enemy->mirror)
    {
        enemy->moveInterp.x = -enemy->moveInterp.x;
    }
}

// FUNCTION: TH07 0x0040fb30
#pragma var_order(b, a)
void EclManager::MathLerp(Enemy *enemy, EclInterp *interp, f32 t)
{
    f32 a = GetFloatVarValue(enemy, interp->args[3].f);
    f32 b = GetFloatVarValue(enemy, interp->args[4].f);

    *GetFloatVar(enemy, &interp->args[7].f, 0, -1) = (b - a) * t + a;
}

#pragma var_order(h11, m1, h01, m0, p1, h10, h00, p0)
// FUNCTION: TH07 0x0040fb90
void EclManager::MathCubicInterp(Enemy *enemy, EclInterp *interp, f32 t)
{
    float h11;
    float m1;
    float h01;
    float m0;
    float p1;
    float h10;
    float h00;
    float p0;

    p0 = GetFloatVarValue(enemy, interp->args[3].f);
    p1 = GetFloatVarValue(enemy, interp->args[4].f);
    m0 = GetFloatVarValue(enemy, interp->args[5].f);
    m1 = GetFloatVarValue(enemy, interp->args[6].f);

    h00 = (t - 1.0f) * (t - 1.0f) * (2.0f * t + 1.0f);
    h01 = (t * t) * (3.0f - 2.0f * t);
    h10 = (1.0f - t) * (1.0f - t) * t;
    h11 = (t - 1.0f) * t * t;

    *GetFloatVar(enemy, &interp->args[7].f, 0, -1) = h00 * p0 +
                                                     h01 * p1 +
                                                     h10 * m0 +
                                                     h11 * m1;
}

#pragma var_order(i, spellcardName, catk, j, nameCsum, newCsum)
// FUNCTION: TH07 0x0040fc90
void EclManager::BeginSpellcard(Enemy *enemy, EclRawInstr *instr)
{
    i32 newCsum;
    i32 nameCsum;
    i32 j;
    Catk *catk;
    char spellcardName[48];
    i32 i;

    memcpy(spellcardName, &instr->args[1], sizeof(spellcardName));
    for (i = 0; (u32)i < 0x30; i++)
    {
        spellcardName[i] = (u8)spellcardName[i] ^ 0xaa;
    }
    g_Gui.ShowSpellcard(instr->args[0].s[0], spellcardName);
    g_BulletManager.RemoveAllBullets(1);
    g_Stage.spellCardState = 1;
    g_Stage.ticksSinceSpellcardStarted = 0;
    for (i = 0; i < g_Stage.numSpellcardVms; i++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &g_Stage.spellcardVms[i], i + g_Stage.spellcardVmsIdx + 0x2dc);
    }
    g_EnemyManager.spellcardInfo.isActive = 1;
    g_EnemyManager.spellcardInfo.isCapturing = 1;
    g_EnemyManager.spellcardInfo.spellcardIdx =
        instr->args[0].us[1];
    g_EnemyManager.spellcardInfo.captureScore =
        g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx];
    g_EnemyManager.spellcardInfo.grazeBonusScore = 0;
    g_EnemyManager.spellcardInfo.scoreDrainRate =
        (i32)g_EnemyManager.spellcardInfo.captureScore /
        (enemy->timerCallbackThreshold / 60 + 10);
    g_EnemyManager.timer = 0;
    enemy->bulletRankSpeedLow = -0.5f;
    enemy->bulletRankSpeedHigh = 0.5f;
    enemy->bulletRankAmount1Low = 0;
    enemy->bulletRankAmount1High = 0;
    enemy->bulletRankAmount2Low = 0;
    enemy->bulletRankAmount2High = 0;
    enemy->specialEffect =
        g_EffectManager.SpawnEffect(0x19, &enemy->position, 1, 1, 0xffffffff);
    enemy->specialEffect->vm.interpStartTimes[4] = 0;
    enemy->specialEffect->vm.interpEndTimes[4] = enemy->timerCallbackThreshold;
    enemy->specialEffect->vm.interpModes[4] = 0;
    enemy->specialEffect->vm.scaleInterpInitial = enemy->specialEffect->vm.scale;
    enemy->specialEffect->vm.scaleInterpFinal.x = 0.125;
    enemy->specialEffect->vm.scaleInterpFinal.y = 0.125;
    enemy->specialEffect->pos1 = enemy->position;
    enemy->customSpecialEffectPos = 0;
    if (g_GameManager.replay == 0)
    {
        catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx];
        nameCsum = 0;
        strcpy(catk->name, spellcardName);
        j = (i32)strlen(catk->name);
        while (0 < j)
        {
            j--;
            nameCsum += catk->name[j];
        }
        newCsum = nameCsum;
        for (j = 0; j < 7; j++)
        {
            nameCsum += catk->numSuccessesPerShot[j];
            nameCsum += catk->numAttemptsPerShot[j];
            nameCsum += catk->highScorePerShot[j];
        }
        if (catk->nameCsum != (u8)nameCsum)
        {
            for (j = 0; j < 7; j++)
            {
                catk->numSuccessesPerShot[j] = 0;
                catk->numAttemptsPerShot[j] = 0;
                catk->highScorePerShot[j] = 0;
            }
        }
        if (catk->numAttemptsPerShot[g_GameManager.shotTypeAndCharacter] < 9999)
        {
            catk->numAttemptsPerShot[g_GameManager.shotTypeAndCharacter]++;
        }
        if (catk->numAttemptsPerShot[6] < 9999)
        {
            catk->numAttemptsPerShot[6]++;
        }
        for (j = 0; j < 7; j++)
        {
            newCsum += catk->numSuccessesPerShot[j];
            newCsum += catk->numAttemptsPerShot[j];
            newCsum += catk->highScorePerShot[j];
        }
        catk->nameCsum = (u8)newCsum;
    }
}

#pragma var_order(score, catk, i, nameCsum, newCsum, j)
// FUNCTION: TH07 0x004101a0
void EclManager::EndSpellcard(Enemy *enemy, EclRawInstr *instr)
{
    i32 j;
    i32 newCsum;
    i32 nameCsum;
    u32 character;
    i32 i;
    Catk *catk;
    i32 score;

    if (g_EnemyManager.spellcardInfo.isActive != 0)
    {
        g_Gui.EndEnemySpellcard();
        if (g_EnemyManager.spellcardInfo.isActive == 1)
        {
            score = g_BulletManager.DespawnBullets(8000, 1);
            score = g_EnemyManager.RemoveAllEnemies(8000, score);
            if (score != 0)
            {
                g_GameManager.AddScore(score);
                g_Gui.ShowBonusScore(score);
            }
            if (g_EnemyManager.spellcardInfo.isCapturing != 0)
            {
                catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx];
                score = g_EnemyManager.spellcardInfo.captureScore +
                        g_EnemyManager.spellcardInfo.grazeBonusScore;
                g_Gui.ShowSpellcardBonus(score);
                g_GameManager.AddScore(score);
                if (g_GameManager.replay == 0)
                {
                    nameCsum = 0;
                    i = strlen(catk->name);
                    while (0 < i)
                    {
                        i--;
                        nameCsum += catk->name[i];
                    }
                    newCsum = nameCsum;
                    for (i = 0; i < 7; i++)
                    {
                        nameCsum += catk->numSuccessesPerShot[i];
                        nameCsum += catk->numAttemptsPerShot[i];
                        nameCsum += catk->highScorePerShot[i];
                    }
                    if (catk->nameCsum != (u8)nameCsum)
                    {
                        for (i = 0; i < 7; i++)
                        {
                            catk->numSuccessesPerShot[i] = 0;
                            catk->numAttemptsPerShot[i] = 0;
                            catk->highScorePerShot[i] = 0;
                        }
                    }
                    character = g_GameManager.shotTypeAndCharacter;
                    if (catk->highScorePerShot[character] < score)
                    {
                        catk->highScorePerShot[character] = score;
                    }
                    if (catk->highScorePerShot[6] < score)
                    {
                        catk->highScorePerShot[6] = score;
                    }
                    if (catk->numSuccessesPerShot[character] < 9999)
                    {
                        catk->numSuccessesPerShot[character]++;
                    }
                    if (catk->numSuccessesPerShot[6] < 9999)
                    {
                        catk->numSuccessesPerShot[6]++;
                    }
                    for (i = 0; i < 7; i++)
                    {
                        newCsum += catk->numSuccessesPerShot[i];
                        newCsum += catk->numAttemptsPerShot[i];
                        newCsum += catk->highScorePerShot[i];
                    }
                    catk->nameCsum = (u8)newCsum;
                }
                g_GameManager.globals->spellCardsCaptured++;
            }
        }
        g_EnemyManager.spellcardInfo.isActive = 0;
        for (j = 0; j < 8; j++)
        {
            if (g_EnemyManager.bosses[j] != NULL &&
                g_EnemyManager.bosses[j]->specialEffect != NULL)
            {
                g_EnemyManager.bosses[j]->specialEffect->inUseFlag = 0;
                g_EnemyManager.bosses[j]->specialEffect = NULL;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END, 0);
    }
    g_Stage.spellCardState = 0;
}

#pragma var_order(local_8, instr, local_10, local_14, local_18, local_1c,     \
                  local_20, local_24, local_28, local_30, local_34, local_38, \
                  local_3c, local_40, local_4c, local_50, local_54, local_60, \
                  local_64, local_68, local_74, local_a0, local_cc, local_d8, \
                  local_e8, local_f0, local_f4, local_fc, uVar6, fVar3,       \
                  fVar28, bVar7)
// FUNCTION: TH07 0x00410520
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    i32 iVar13;
    u32 uVar17;
    i32 iVar19;
    f32 fVar8;

    EclInterp *local_fc;
    f32 local_f4;
    i32 local_f0;
    f32 local_e8;
    D3DXVECTOR3 local_d8;
    D3DXVECTOR3 local_cc;
    D3DXVECTOR3 local_a0;
    D3DXVECTOR3 local_74;
    i32 local_68;
    i32 local_64;
    D3DXVECTOR3 local_60;
    i32 local_54;
    i32 local_50;
    D3DXVECTOR3 local_4c;
    i32 local_40;
    i32 local_3c;
    f32 local_38;
    AnyArg *local_34;
    i32 local_30;
    EnemyLaserShooter *local_28;
    BulletCommand *local_24;
    EnemyBulletShooter *local_20;
    AnyArg *local_1c;
    EclInterp *local_18;
    i32 local_14;
    f32 local_10;
    EclRawInstr *instr;
    i32 local_8;
    bool bVar7;
    f32 fVar28;
    f32 fVar3;
    u8 uVar6;

restart:
    instr = enemy->currentContext.curInstr;
    if (enemy->runInterrupt >= 0)
    {
        goto handle_interrupt;
    }

    if (enemy->periodicCallbackSub >= 0)
    {
        enemy->periodicCounter++;
        if (enemy->periodicCounter.GetCurrent() >= enemy->periodicTimer.GetCurrent())
        {
            enemy->periodicCounter = 0;
            enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
            enemy->currentContext.eclContextArgs = enemy->savedEclContextArgs;
            g_EclManager.CallEclSub(&enemy->currentContext,
                                    (i16)enemy->periodicCallbackSub);
            if (enemy->stackDepth < 0xf)
            {
                enemy->stackDepth++;
            }
            instr = enemy->currentContext.curInstr;
            enemy->currentContext.isPeriodicSub = 1;
        }
    }
    for (;;)
    {
        if (enemy->currentContext.timer2.GetCurrent() > 0)
        {
            enemy->currentContext.timer2--;
            enemy->currentContext.time--;
            break;
        }
        if (enemy->currentContext.time == instr->time)
        {
            if ((instr->skipInstrOnDifficulty & g_GameManager.difficultyMask) == 0)
            {
                goto skip;
            }
            switch (instr->id)
            {
            case 1:
                return ZUN_ERROR;
            case 0x2d:
                enemy->currentContext.timer2 = GET_INT_VALUE(enemy, 0);
                break;
            case 3:
                *GET_INT_PTR(enemy, 2) -= 1;
                if (GET_INT_VALUE(enemy, 2) <= 0)
                {
                    break;
                }
            case 2:
                enemy->currentContext.time.current = instr->args[0].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                continue;
            case 4:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1);
                break;
            case 5:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1);
                break;
            case 0x28:
                *GET_FLOAT_PTR(enemy, 0) =
                    utils::AddNormalizeAngle(GET_FLOAT_VALUE(enemy, 0), 0.0f);
                break;
            case 6:
                *GET_INT_PTR(enemy, 0) =
                    g_Rng.GetRandomU32InRange(GET_INT_VALUE(enemy, 1));
                break;
            case 7:
                *GET_INT_PTR(enemy, 0) =
                    g_Rng.GetRandomU32InRange(GET_INT_VALUE(enemy, 1)) +
                    GET_INT_VALUE(enemy, 2);
                break;
            case 8:
                *GET_FLOAT_PTR(enemy, 0) =
                    g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 1));
                break;
            case 9:
                *GET_FLOAT_PTR(enemy, 0) =
                    g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 1)) +
                    GET_FLOAT_VALUE(enemy, 2);
                break;
            case 10:
                *GET_INT_PTR(enemy, 0) =
                    (((g_Rng.GetRandomU16() & 1) != 0 ? 2 : 0) - 1) *
                    GET_INT_VALUE(enemy, 1);
                break;
            case 0xb:
                *GET_FLOAT_PTR(enemy, 0) =
                    ((g_Rng.GetRandomU16() & 1) != 0 ? 1.0f : -1.0f) * GET_FLOAT_VALUE(enemy, 1);
                break;
            case 0x11:
                *GET_INT_PTR(enemy, 0) += 1;
                break;
            case 0x12:
                *GET_INT_PTR(enemy, 0) -= 1;
                break;
            case 0x2b:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(
                        g_EnemyManager.bosses[GET_INT_VALUE(enemy, 2)], 1);
                break;
            case 0x2c:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(
                        g_EnemyManager.bosses[GET_INT_VALUE(enemy, 2)], 1);
                break;
            case 0xc:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) + GET_INT_VALUE(enemy, 2);
                break;
            case 0x13:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) + GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0xd:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) - GET_INT_VALUE(enemy, 2);
                break;
            case 20:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) - GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0xe:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) * GET_INT_VALUE(enemy, 2);
                break;
            case 0x15:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) * GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0xf:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) / GET_INT_VALUE(enemy, 2);
                break;
            case 0x16:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) / GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0x10:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) % GET_INT_VALUE(enemy, 2);
                break;
            case 0x17:
                *GET_FLOAT_PTR(enemy, 0) =
                    fmodf(GET_FLOAT_VALUE(enemy, 1), GET_FLOAT_VALUE(enemy, 2));
                break;
            case 0x18:
                *GET_FLOAT_PTR(enemy, 0) =
                    sinf(GET_FLOAT_VALUE(enemy, 1));
                break;
            case 0x19:
                *GET_FLOAT_PTR(enemy, 0) =
                    cosf(GET_FLOAT_VALUE(enemy, 1));
                break;
            case 0x1a:
                *GET_FLOAT_PTR(enemy, 0) =
                    atan2f(GET_FLOAT_VALUE(enemy, 4) - GET_FLOAT_VALUE(enemy, 2),
                           GET_FLOAT_VALUE(enemy, 3) - GET_FLOAT_VALUE(enemy, 1));
                break;
            case 0x9f:
                local_10 = GET_FLOAT_VALUE(enemy, 1) -
                           GET_FLOAT_VALUE(enemy, 2);
                *GET_FLOAT_PTR(enemy, 0) =
                    local_10 * GET_FLOAT_VALUE(enemy, 3) +
                    GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0x1b:
                local_18 = enemy->currentContext.interps;
                for (local_14 = 0; local_14 < 8; local_14++, local_18++)
                {
                    if ((local_18->fn == NULL) ||
                        (local_18->args[7].f == instr->args[0].f))
                    {
                        (local_18->timer) = 0;
                        local_18->args[7] = instr->args[0];
                        local_18->args[0].i = GET_INT_VALUE(enemy, 1);
                        local_18->args[1].i = GET_INT_VALUE(enemy, 2);
                        local_18->args[2].i = GET_INT_VALUE(enemy, 3);
                        local_18->fn = g_EclInterpFuncs[local_18->args[1].i];
                        local_18->args[3].f = GET_FLOAT_VALUE(enemy, 4);
                        local_18->args[4].f = GET_FLOAT_VALUE(enemy, 5);
                        local_18->args[5].f = GET_FLOAT_VALUE(enemy, 6);
                        local_18->args[6].f = GET_FLOAT_VALUE(enemy, 7);
                        break;
                    }
                }
                break;
            case 0x1c:
                if (GET_INT_VALUE(enemy, 0) == GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x1d:
                if (GET_FLOAT_VALUE(enemy, 0) == GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x1e:
                if (GET_INT_VALUE(enemy, 0) != GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x1f:
                if (GET_FLOAT_VALUE(enemy, 0) != GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x20:
                if (GET_INT_VALUE(enemy, 0) < GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x21:
                if (GET_FLOAT_VALUE(enemy, 0) < GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x22:
                if (GET_INT_VALUE(enemy, 0) <= GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x23:
                if (GET_FLOAT_VALUE(enemy, 0) <= GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x24:
                if (GET_INT_VALUE(enemy, 0) > GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x25:
                if (GET_FLOAT_VALUE(enemy, 0) > GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x26:
                if (GET_INT_VALUE(enemy, 0) >= GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 0x27: {
                if (GET_FLOAT_VALUE(enemy, 0) >= GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            LAB_00411f00:
                enemy->currentContext.time.current = instr->args[2].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[3].i);
                continue;
            case 0x29:
                local_8 = instr->args[0].i;
                enemy->currentContext.curInstr =
                    (EclRawInstr *)((u8 *)instr + instr->size);
                if (!enemy->noStackRet)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext, (i16)local_8);
                enemy->currentContext.eclContextArgs.globalVars = g_GlobalEclVars;
                if ((!enemy->noStackRet) && (enemy->stackDepth < 0xf))
                {
                    enemy->stackDepth++;
                }
                goto restart;
            case 0x2a:
                if (enemy->noStackRet)
                {
                    // STRING: TH07 0x004986e4
                    DebugPrint("error : no Stack Ret\r\n");
                }
                enemy->stackDepth--;
                if (enemy->currentContext.isPeriodicSub != 0)
                {
                    enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
                    enemy->currentContext.isPeriodicSub = 0;
                }
                enemy->currentContext = enemy->savedContextStack[enemy->stackDepth];
                goto restart;
            case 0x5f:
                g_AnmManager->SetAnmIdxAndExecuteScript(&enemy->primaryVm,
                                                        GET_INT_VALUE(enemy, 0) + 0x900);
                break;
            case 0x61:
                if (1 < GET_INT_VALUE(enemy, 0))
                {
                    // STRING: TH07 0x004986c8
                    DebugPrint("error : sub anim overflow\r\n");
                }
                if (GET_INT_VALUE(enemy, 1) >= 0)
                {
                    g_AnmManager->SetAnmIdxAndExecuteScript(
                        &enemy->vms[GET_INT_VALUE(enemy, 0)],
                        GET_INT_VALUE(enemy, 1) + 0x900);
                }
                else
                {
                    enemy->vms[GET_INT_VALUE(enemy, 0)].anmFileIdx = -1;
                }
                break;
            case 0x2e:
                enemy->position.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->position.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->position.z = GET_FLOAT_VALUE(enemy, 2);
                enemy->ClampPos();
                break;
            case 0x2f:
                enemy->axisSpeed.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->axisSpeed.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->axisSpeed.z = GET_FLOAT_VALUE(enemy, 2);
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                enemy->flags1 &= 0xfc;
                break;
            case 0x30:
                enemy->angularVelocity = GET_FLOAT_VALUE(enemy, 0);
                enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
                break;
            case 0x35:
                enemy->angle = g_Player.AngleToPlayer(&enemy->position) +
                               GET_FLOAT_VALUE(enemy, 0);
                enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 1);
                enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
                break;
            case 0x31:
                enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 0);
                enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
                break;
            case 0x32:
                enemy->moveAcceleration = GET_FLOAT_VALUE(enemy, 0);
                enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
                break;
            case 0x3b:
                enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 0x3c:
                enemy->flags1 = enemy->flags1 | 3;
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 0x3d:
                enemy->flags1 = (enemy->flags1 & 0xfc) | 2;
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x43:
            case 0x44:
            case 0x45:
            case 0x46:
            case 0x47:
            case 0x48:
                if (0 < enemy->life)
                {
                    local_1c = instr->args;
                    local_20 = &enemy->bulletProps;
                    local_8 = (instr->paramMask & 1) != 0
                                  ? GetVarValue(enemy, local_1c->s[0])
                                  : local_1c->s[0];
                    local_20->sprite = local_8;
                    local_20->aimMode = instr->id - 0x40;
                    local_20->count1 = GET_INT_VALUE_D(enemy, 1, 2);
                    local_20->count2 = GET_INT_VALUE_D(enemy, 2, 3);
                    local_20->position = enemy->position + enemy->shootOffset;
                    local_20->angle1 = GET_FLOAT_VALUE_D(enemy, 5, 6);
                    local_20->speed1 = GET_FLOAT_VALUE_D(enemy, 3, 4);
                    local_20->angle2 = GET_FLOAT_VALUE_D(enemy, 6, 7);
                    local_20->speed2 = GET_FLOAT_VALUE_D(enemy, 4, 5);
                    if (g_EnemyManager.spellcardInfo.isActive == 0)
                    {
                        iVar13 = enemy->bulletRankAmount1Low;
                        iVar19 = ((i32)enemy->bulletRankAmount1High - iVar13) *
                                 g_GameManager.rank.rank;
                        local_20->count1 += iVar13 + (i16)(i32)(iVar19 / 32);
                        if (local_20->count1 <= 0)
                        {
                            local_20->count1 = 1;
                        }
                        iVar13 = enemy->bulletRankAmount1Low;
                        iVar19 = ((i32)enemy->bulletRankAmount2High - iVar13) *
                                 g_GameManager.rank.rank;
                        local_20->count2 += iVar13 + (i16)(i32)(iVar19 / 32);
                        if (local_20->count2 <= 0)
                        {
                            local_20->count2 = 1;
                        }
                        fVar28 = enemy->bulletRankSpeedLow;
                        local_20->speed1 +=
                            ((enemy->bulletRankSpeedHigh - fVar28) *
                             (f32)g_GameManager.rank.rank) /
                                32.0f +
                            fVar28;
                        if (local_20->speed1 < 0.3f)
                        {
                            local_20->speed1 = 0.3f;
                        }
                        fVar28 = enemy->bulletRankSpeedLow;
                        local_20->speed2 +=
                            (((enemy->bulletRankSpeedHigh - fVar28) *
                              (f32)g_GameManager.rank.rank) /
                                 32.0f +
                             fVar28) /
                            2.0f;
                        if (local_20->speed2 < 0.3f)
                        {
                            local_20->speed2 = 0.3f;
                        }
                    }
                    local_20->unused_c2 = 0;
                    local_20->flags = local_1c[7].u;
                    local_8 = (instr->paramMask & 2) != 0
                                  ? GetVarValue(enemy, local_1c->s[1])
                                  : local_1c->s[1];
                    local_20->spriteOffset = local_8;
                    if ((enemy->flags1 >> 5 & 1) == 0)
                    {
                        g_BulletManager.SpawnBulletPattern(local_20);
                    }
                }
                break;
            case 0x4f:
                local_24 =
                    &enemy->bulletProps.commands[GET_INT_VALUE(enemy, 0)];
                local_24->speed = GET_FLOAT_VALUE(enemy, 5);
                local_24->angle = GET_FLOAT_VALUE(enemy, 6);
                local_24->duration = GET_INT_VALUE(enemy, 3);
                local_24->loopCount = GET_INT_VALUE(enemy, 4);
                local_24->type = GET_INT_VALUE(enemy, 1);
                local_24->flag = GET_INT_VALUE(enemy, 2);
                break;
            case 0x62:
                enemy->deathAnm1 = instr->args[0].c[0];
                enemy->deathAnm2 = instr->args[0].c[1];
                enemy->deathAnm3 = instr->args[0].c[2];
                break;
            case 0x49:
                enemy->shootInterval = GET_INT_VALUE(enemy, 0);
                if (enemy->shootInterval != 0)
                {
                    iVar19 = enemy->shootInterval / 5;
                    iVar13 =
                        (-enemy->shootInterval / 5 - iVar19) * g_GameManager.rank.rank;
                    enemy->shootInterval =
                        (i32)(iVar13 / 32) + iVar19 + enemy->shootInterval;
                    enemy->shootIntervalTimer = 0;
                }
                break;
            case 0x4a:
                enemy->shootInterval = GET_INT_VALUE(enemy, 0);
                if (enemy->shootInterval != 0)
                {
                    iVar19 = enemy->shootInterval / 5;
                    iVar13 =
                        (-enemy->shootInterval / 5 - iVar19) * g_GameManager.rank.rank;
                    enemy->shootInterval =
                        (i32)(iVar13 / 32) + iVar19 + enemy->shootInterval;
                    enemy->shootIntervalTimer =
                        g_Rng.GetRandomU32InRange(enemy->shootInterval);
                }
                break;
            case 0x4b:
                enemy->flags1 |= 0x20;
                break;
            case 0x4c:
                enemy->flags1 &= 0xdf;
                break;
            case 0x4d:
                enemy->bulletProps.position = enemy->position + enemy->shootOffset;
                g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
                break;
            case 0x4e:
                enemy->shootOffset.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->shootOffset.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->shootOffset.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0x52:
            case 0x53:
                local_28 = &enemy->laserProps;
                local_28->position = enemy->position + enemy->shootOffset;
                local_28->sprite = instr->args[0].s[0];
                local_28->spriteOffset = (instr->paramMask & 2) != 0
                                             ? (i16)GetVarValue(enemy, (i32)instr->args[0].s[1])
                                             : instr->args[0].s[1];
                local_28->angle1 = GET_FLOAT_VALUE_D(enemy, 1, 2);
                local_28->speed1 = GET_FLOAT_VALUE_D(enemy, 2, 3);
                local_28->startOffset = GET_FLOAT_VALUE_D(enemy, 3, 4);
                local_28->endOffset = GET_FLOAT_VALUE_D(enemy, 4, 5);
                local_28->startLength = GET_FLOAT_VALUE_D(enemy, 5, 6);
                local_28->width = instr->args[6].f;
                local_28->startTime = instr->args[7].i;
                local_28->duration = instr->args[8].i;
                local_28->endTime = instr->args[9].i;
                local_28->grazeDelay = instr->args[10].i;
                local_28->aimMode = instr->args[11].i;
                local_28->flags = instr->args[12].u;
                local_28->type = (instr->id == 0x53) ? 0 : 1;
                enemy->lasers[enemy->laserIdx] =
                    g_BulletManager.SpawnLaserPattern(local_28);
                break;
            case 0x54:
                enemy->laserIdx = GET_INT_VALUE(enemy, 0);
                break;
            case 0x55:
                if (enemy->lasers[local_8 = GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    enemy->lasers[local_8]->angle = utils::AddNormalizeAngle(
                        enemy->lasers[local_8]->angle, GET_FLOAT_VALUE(enemy, 1));
                }
                break;
            case 0x98:
                if (enemy->lasers[local_8 = GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    enemy->lasers[local_8]->angle = GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case 0x56:
                if (enemy->lasers[local_8 = GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    enemy->lasers[local_8]->angle =
                        g_Player.AngleToPlayer(&enemy->lasers[local_8]->pos) +
                        GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case 0x57:
                if (enemy->lasers[local_8 = GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    enemy->lasers[local_8]->pos.x =
                        GET_FLOAT_VALUE(enemy, 1) + enemy->position.x;
                    enemy->lasers[local_8]->pos.y =
                        GET_FLOAT_VALUE(enemy, 2) + enemy->position.y;
                    enemy->lasers[local_8]->pos.z =
                        GET_FLOAT_VALUE(enemy, 3) + enemy->position.z;
                }
                break;
            case 0x9c:
                if (enemy->lasers[local_8 = GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    enemy->lasers[local_8]->hideWarning =
                        GET_INT_VALUE(enemy, 1);
                }
                break;
            case 0x58:
                local_8 = GET_INT_VALUE(enemy, 0);
                if ((enemy->lasers[local_8] == NULL) ||
                    (enemy->lasers[local_8]->inUse == 0))
                {
                    enemy->currentContext.compareRegister = 1;
                }
                else
                {
                    enemy->currentContext.compareRegister = 0;
                }
                break;
            case 0x59:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (((enemy->lasers[local_8] != NULL) &&
                     (enemy->lasers[local_8]->inUse != 0)) &&
                    (enemy->lasers[local_8]->state < 2))
                {
                    enemy->lasers[local_8]->state = 2;
                    enemy->lasers[local_8]->timer = 0;
                    enemy->lasers[local_8]->width =
                        enemy->lasers[local_8]->targetWidth;
                }
                break;
            case 0x86:
                for (local_30 = 0; local_30 < 0x20; local_30 += 1)
                {
                    enemy->lasers[local_30] = NULL;
                }
                break;
            case 0x9d:
                if (enemy->lasers[local_8 = GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    enemy->lasers[local_8]->startLength =
                        GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case 0x9e:
                if (enemy->lasers[local_8 = GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    enemy->lasers[local_8]->startOffset =
                        GET_FLOAT_VALUE(enemy, 1);
                    enemy->lasers[local_8]->endOffset =
                        GET_FLOAT_VALUE(enemy, 2);
                }
                break;
            case 0x93:
                g_EnemyManager.unused_9545f0 = GET_INT_VALUE(enemy, 0);
                break;
            case 0x63:
                if (GET_INT_VALUE(enemy, 0) < 0)
                {
                    if (enemy->bossId < 4)
                    {
                        g_Gui.bossPresent = 0;
                    }
                    g_EnemyManager.bosses[enemy->bossId] = NULL;
                    enemy->isBoss = 0;
                    g_AsciiManager.bossMarkers[enemy->bossId].pendingInterrupt = 2;
                    enemy->ResetEffectArray();
                }
                else
                {
                    g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)] = enemy;
                    g_Gui.bossPresent = 1;
                    g_Gui.bossHealthBar = 1.0f;
                    enemy->isBoss = 1;
                    enemy->bossId = GET_INT_VALUE(enemy, 0);
                    g_AsciiManager.bossMarkers[enemy->bossId].pendingInterrupt = 1;
                }
                break;
            case 0x64:
                local_34 = instr->args;
                enemy->effects[enemy->effectsNum] = g_EffectManager.SpawnParticles(
                    0xd, &enemy->position, 1, g_BulletColor[local_34->i]);
                enemy->effects[enemy->effectsNum]->direction.x = local_34[1].f;
                enemy->effects[enemy->effectsNum]->direction.y = local_34[2].f;
                enemy->effects[enemy->effectsNum]->direction.z = local_34[3].f;
                enemy->effectDistance = local_34[4].f;
                enemy->effectsNum++;
                break;
            case 0x36:
                if (GET_INT_VALUE(enemy, 0) < 1)
                {
                    enemy->angle =
                        utils::AddNormalizeAngle(
                            GET_FLOAT_VALUE(enemy, 2), 0.0f);
                    enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 3);
                    enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
                    enemy->moveInterpTimer = enemy->moveInterpStartTime =
                        GET_INT_VALUE(enemy, 0);
                }
                else
                {
                    MoveDirTime(enemy, instr);
                }
                break;
            case 0x37:
                MovePosTime(enemy, instr);
                break;
            case 0x38:
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                enemy->moveInterpStartPos.x = GET_FLOAT_VALUE(enemy, 1);
                enemy->moveInterpStartPos.y = GET_FLOAT_VALUE(enemy, 2);
                enemy->moveInterpStartPos.z = GET_FLOAT_VALUE(enemy, 3);
                enemy->moveAngle = GET_FLOAT_VALUE(enemy, 4);
                enemy->moveAngularVelocity = GET_FLOAT_VALUE(enemy, 5);
                enemy->moveRadius = GET_FLOAT_VALUE(enemy, 6);
                enemy->moveRadialVelocity = GET_FLOAT_VALUE(enemy, 7);
                enemy->flags1 |= 3;
                break;
            case 0x39:
                enemy->moveRadius = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveRadialVelocity = GET_FLOAT_VALUE(enemy, 1);
                break;
            case 0x3a:
                enemy->moveAngle = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveAngularVelocity = GET_FLOAT_VALUE(enemy, 1);
                break;
            case 0x3e:
                enemy->lowerMoveLimit.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->lowerMoveLimit.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->upperMoveLimit.x = GET_FLOAT_VALUE(enemy, 2);
                enemy->upperMoveLimit.y = GET_FLOAT_VALUE(enemy, 3);
                enemy->hasMovementBounds = 1;
                break;
            case 0x3f:
                enemy->hasMovementBounds = 0;
                break;
            case 0x33:
                *GET_FLOAT_PTR(enemy, 0) =
                    g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 2) -
                                                GET_FLOAT_VALUE(enemy, 1)) +
                    GET_FLOAT_VALUE(enemy, 1);
                break;
            case 0x34:
                local_38 =
                    (g_Player.positionCenter.x < enemy->position.x)
                        ? utils::AddNormalizeAngle(
                              g_Rng.GetRandomFloatInRange(1.5707964f) + 2.3561945f, 0.0f)
                        : g_Rng.GetRandomFloatInRange(1.5707964f) - 0.7853982f;
                if (enemy->position.x < enemy->lowerMoveLimit.x + 96.0)
                {
                    if (local_38 <= 1.5707964f)
                    {
                        if (local_38 < -1.5707964f)
                        {
                            local_38 = -3.1415927f - local_38;
                        }
                    }
                    else
                    {
                        local_38 = 3.1415927f - local_38;
                    }
                }
                if (enemy->upperMoveLimit.x - 96.0f < enemy->position.x)
                {
                    if ((1.5707964f <= local_38) || (local_38 < 0.0f))
                    {
                        if (-1.5707964f < local_38 && local_38 <= 0.0f)
                        {
                            local_38 = -3.1415927f - local_38;
                        }
                    }
                    else
                    {
                        local_38 = 3.1415927f - enemy->angle;
                    }
                }
                if ((enemy->lowerMoveLimit.y + 48.0f > enemy->position.y) &&
                    (local_38 < 0.0f))
                {
                    local_38 = -local_38;
                }
                if ((enemy->upperMoveLimit.y - 48.0f < enemy->position.y) &&
                    (0.0f < local_38))
                {
                    local_38 = -local_38;
                }
                *GET_FLOAT_PTR(enemy, 0) = local_38;
                break;
            case 0x60:
                enemy->anmExDefaults = instr->args[0].s[0];
                enemy->anmExFarLeft = instr->args[0].s[1];
                enemy->anmExFarRight = instr->args[1].s[0];
                enemy->anmExLeft = instr->args[1].s[1];
                enemy->anmExRight = instr->args[2].s[0];
                enemy->anmExFlags = 0xff;
                break;
            case 0x65:
                enemy->hitboxSize.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->hitboxSize.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->hitboxSize.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0x99:
                enemy->grazeSize.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->grazeSize.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->grazeSize.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case 0x66:
                enemy->hasContactHitbox = instr->args[0].b[0];
                break;
            case 0x67:
                enemy->canBeDamaged = instr->args[0].b[0];
                break;
            case 0x68:
                enemy->isHittable = instr->args[0].b[0];
                break;
            case 0x69:
                g_SoundPlayer.PlaySoundByIdx(GET_INT_VALUE(enemy, 0), 0);
                break;
            case 0x6a:
                enemy->deathType = instr->args[0].b[0];
                break;
            case 0x6b:
                enemy->deathCallbackSub = (u32)instr->args[0].b[0];
                break;
            case 0x6c:
                enemy->interrupts[GET_INT_VALUE(enemy, 1)] =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 0x6d:
                enemy->runInterrupt = GET_INT_VALUE(enemy, 0);
            handle_interrupt:
                enemy->currentContext.curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
                if (!enemy->noStackRet)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext,
                                        enemy->interrupts[enemy->runInterrupt]);
                if (enemy->stackDepth < 0xf)
                {
                    enemy->stackDepth = enemy->stackDepth + 1;
                }
                enemy->runInterrupt = -1;
                goto restart;
            case 0x6e:
                enemy->life = enemy->maxLife = GET_INT_VALUE(enemy, 0);
                if ((enemy->bossId == 0) && enemy->isBoss)
                {
                    for (local_3c = 0; local_3c < 8; local_3c += 1)
                    {
                        g_Gui.bossHealthEased[local_3c] = 0.0f;
                        g_Gui.bossHealth[local_3c] = 0.0f;
                    }
                }
                break;
            case 0x8b:
                local_40 = GET_INT_VALUE(enemy, 0);
                g_Gui.bossHealthEased[local_40] =
                    (f32)GET_INT_VALUE(enemy, 1) /
                    (f32)enemy->maxLife;
                g_Gui.bossHealth[local_40] =
                    (f32)GET_INT_VALUE(enemy, 2) /
                    (f32)enemy->maxLife;
                g_Gui.bossColor[local_40] = GET_INT_VALUE(enemy, 3);
                break;
            case 0x5a:
                BeginSpellcard(enemy, instr);
                break;
            case 0x5b:
                EndSpellcard(enemy, instr);
                break;
            case 0x6f:
                enemy->timer = GET_INT_VALUE(enemy, 0);
                break;
            case 0x70:
                enemy->lifeCallbackThreshold[0] = GET_INT_VALUE(enemy, 0);
                break;
            case 0x71:
                enemy->lifeCallbackSub[0] = GET_INT_VALUE(enemy, 0);
                break;
            case 0x94:
                enemy->lifeCallbackThreshold[GET_INT_VALUE(enemy, 0)] =
                    GET_INT_VALUE(enemy, 1);
                enemy->lifeCallbackSub[GET_INT_VALUE(enemy, 0)] =
                    GET_INT_VALUE(enemy, 2);
                break;
            case 0x72:
                enemy->timerCallbackThreshold = GET_INT_VALUE(enemy, 0);
                enemy->timer = 0;
                break;
            case 0x73:
                enemy->timerCallbackSub = GET_INT_VALUE(enemy, 0);
                break;
            case 0x90:
                enemy->periodicTimer = GET_INT_VALUE(enemy, 0);
                enemy->periodicCallbackSub = GET_INT_VALUE(enemy, 1);
                enemy->periodicCounter = 0;
                enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
                break;
            case 0x74:
                enemy->canDie = instr->args[0].b[0];
                break;
            case 0x75:
                g_EffectManager.SpawnParticles(
                    GET_INT_VALUE(enemy, 0),
                    &enemy->position,
                    GET_INT_VALUE(enemy, 1),
                    *(D3DCOLOR *)GET_INT_PTR(enemy, 2));
                break;
            case 0x76:
                local_4c.x = GET_FLOAT_VALUE(enemy, 3);
                local_4c.y = GET_FLOAT_VALUE(enemy, 4);
                local_4c.z = GET_FLOAT_VALUE(enemy, 5);
                g_EffectManager.SpawnMovingParticles(
                    GET_INT_VALUE(enemy, 0),
                    &enemy->position,
                    &local_4c,
                    GET_INT_VALUE(enemy, 1),
                    *(D3DCOLOR *)GET_INT_PTR(enemy, 2));
                break;
            case 0x77:
                local_50 = GET_INT_VALUE(enemy, 0);
                for (local_54 = 0; local_54 < local_50; local_54 += 1)
                {
                    local_60 = enemy->position;
                    local_60.x = (g_Rng.GetRandomFloatInRange(128.0f) - 64.0f) + local_60.x;
                    local_60.y = (g_Rng.GetRandomFloatInRange(128.0f) - 64.0f) + local_60.y;
                    if ((i32)g_GameManager.globals->currentPower < 128)
                    {
                        g_ItemManager.SpawnItem(&local_60,
                                                local_54 == 0 ? 2 : 0, 0);
                    }
                    else
                    {
                        g_ItemManager.SpawnItem(&local_60, ITEM_POINT, 0);
                    }
                }
                break;
            case 0x9a:
                local_64 = GET_INT_VALUE(enemy, 0);
                for (local_68 = 0; local_68 < local_64; local_68 += 1)
                {
                    local_74 = enemy->position;
                    local_74.x = (g_Rng.GetRandomFloatInRange(128.0f) - 64.0f) + local_74.x;
                    local_74.y = (g_Rng.GetRandomFloatInRange(128.0f) - 64.0f) + local_74.y;
                    g_ItemManager.SpawnItem(&local_74, ITEM_POINT, 0);
                }
                break;
            case 0x78:
                enemy->primaryVmAutoRotate = instr->args[0].b[0];
                break;
            case 0x79:
                (*g_EclExInstr[GET_INT_VALUE(enemy, 0)])(enemy, instr);
                break;
            case 0x7a:
                if (GET_INT_VALUE(enemy, 0) < 0)
                {
                    enemy->currentContext.func = NULL;
                }
                else
                {
                    enemy->currentContext.func =
                        g_EclExInstr[GET_INT_VALUE(enemy, 0)];
                    enemy->currentContext.eclExInstr = instr;
                }
                break;
            case 0x7b:
                enemy->currentContext.time += GET_INT_VALUE(enemy, 0);
                break;
            case 0x7c:
                g_ItemManager.SpawnItem(&enemy->position,
                                        GET_INT_VALUE(enemy, 0), 0);
                break;
            case 0x7d:
                g_Stage.scriptWaitTime = GET_INT_VALUE(enemy, 0);
                break;
            case 0x7e:
                g_Gui.bossLifeMarkers = GET_INT_VALUE(enemy, 0);
                g_GameManager.playTimeAll += 0x708;
                break;
            case 0x5c:
                if (0 < enemy->life)
                {
                    local_a0.x = GET_FLOAT_VALUE(enemy, 1);
                    local_a0.y = GET_FLOAT_VALUE(enemy, 2);
                    local_a0.z = GET_FLOAT_VALUE(enemy, 3);
                    g_EnemyManager.SpawnEnemyEx(instr->args[0].i, &local_a0,
                                                GET_INT_VALUE(enemy, 4),
                                                GET_INT_VALUE(enemy, 5),
                                                GET_INT_VALUE(enemy, 6),
                                                &enemy->currentContext.eclContextArgs);
                }
                break;
            case 0x5d:
                if (0 < enemy->life)
                {
                    local_cc.x = GET_FLOAT_VALUE(enemy, 1);
                    local_cc.y = GET_FLOAT_VALUE(enemy, 2);
                    local_cc.z = GET_FLOAT_VALUE(enemy, 3);
                    local_cc += enemy->position;
                    g_EnemyManager.SpawnEnemyEx(instr->args[0].i, &local_cc,
                                                GET_INT_VALUE(enemy, 4),
                                                GET_INT_VALUE(enemy, 5),
                                                GET_INT_VALUE(enemy, 6),
                                                &enemy->currentContext.eclContextArgs);
                }
                break;
            case 0x5e:
                g_EnemyManager.RemoveAllEnemies(8000, 0);
                break;
            case 0x80:
                enemy->primaryVm.pendingInterrupt = GET_INT_VALUE(enemy, 0);
                break;
            case 0x81:
                enemy->vms[instr->args[0].i].pendingInterrupt = instr->args[1].s[0];
                break;
            case 0x50:
                g_BulletManager.RemoveAllBullets(1);
                break;
            case 0x51:
                if (GET_INT_VALUE(enemy, 0) < 0)
                {
                    enemy->bulletProps.flags &= 0xfffffdff;
                }
                else
                {
                    enemy->bulletProps.soundIdx = GET_INT_VALUE(enemy, 0);
                    enemy->bulletProps.flags = enemy->bulletProps.flags | 0x200;
                }
                enemy->bulletProps.soundOverride = GET_INT_VALUE(enemy, 1);
                break;
            case 0x82:
                enemy->noStackRet = instr->args[0].b[0];
                break;
            case 0x83:
                enemy->bulletRankSpeedLow = GET_FLOAT_VALUE(enemy, 0);
                enemy->bulletRankSpeedHigh = GET_FLOAT_VALUE(enemy, 1);
                enemy->bulletRankAmount1Low = GET_INT_VALUE(enemy, 2);
                enemy->bulletRankAmount1High = GET_INT_VALUE(enemy, 3);
                enemy->bulletRankAmount2Low = GET_INT_VALUE(enemy, 4);
                enemy->bulletRankAmount2High = GET_INT_VALUE(enemy, 5);
                break;
            case 0x84:
                enemy->hasNoCollision = instr->args[0].b[0];
                break;
            case 0x85:
                enemy->timerCallbackSub = enemy->deathCallbackSub;
                enemy->timer = 0;
                break;
            case 0x87:
                enemy->isSurvivalSpellcard = instr->args[0].b[0];
                break;
            case 0x88:
                enemy->isProjectile = instr->args[0].b[0];
                enemy->zLayer = 2;
                break;
            case 0x89:
                enemy->disableOOBDespawn = instr->args[0].b[0];
                break;
            case 0x8a:
                enemy->trailFlags = instr->args[0].c[0];
                enemy->trailCount = GET_INT_VALUE(enemy, 1);
                enemy->trailInterval = GET_INT_VALUE(enemy, 2);
                enemy->trailNodeStep = GET_INT_VALUE(enemy, 3);
                if ((enemy->trailFlags & 8) != 0)
                {
                    g_AnmManager->UpdateTrail(
                        &enemy->primaryVm, enemy->trailVertices,
                        (i32)enemy->trailCount / (i32)enemy->trailNodeStep << 1);
                }
                break;
            case 0x8c:
                g_EffectManager.globalColorMultiplierR =
                    GET_FLOAT_VALUE(enemy, 0);
                g_EffectManager.globalColorMultiplierG =
                    GET_FLOAT_VALUE(enemy, 1);
                g_EffectManager.globalColorMultiplierB =
                    GET_FLOAT_VALUE(enemy, 2);
                g_EffectManager.globalColorMultiplierA =
                    GET_FLOAT_VALUE(enemy, 3);
                break;
            case 0x8e:
                enemy->invincibilityTimer = GET_INT_VALUE(enemy, 0);
                break;
            case 0x8f:
                g_BulletManager.RemoveBulletsInRadius(&enemy->position,
                                                      GET_FLOAT_VALUE(enemy, 0));
                break;
            case 0x91:
                if (g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)]->runInterrupt = GET_INT_VALUE(enemy, 1);
                }
                break;
            case 0x92:
                g_BulletManager.RemoveAllBullets(0);
                break;
            case 0x95:
                enemy->flags4 = (enemy->flags4 & 0xfd) |
                                ((u8)GET_INT_VALUE(enemy, 0) & 1) << 1;
                if ((enemy->flags4 >> 1 & 1) == 0)
                {
                    enemy->specialEffect->pos1.x = GET_FLOAT_VALUE(enemy, 1);
                    enemy->specialEffect->pos1.y = GET_FLOAT_VALUE(enemy, 2);
                    enemy->specialEffect->pos1.z = GET_FLOAT_VALUE(enemy, 3);
                }
                break;
            case 0x96:
                enemy->primaryVm.rotation.z = GET_FLOAT_VALUE(enemy, 0);
                break;
            case 0x97:
                *GET_FLOAT_PTR(enemy, 1) =
                    sinf(GET_FLOAT_VALUE(enemy, 2)) * GET_FLOAT_VALUE(enemy, 3);
                *GET_FLOAT_PTR(enemy, 0) =
                    cosf(GET_FLOAT_VALUE(enemy, 2)) * GET_FLOAT_VALUE(enemy, 3);
                break;
            case 0x9b:
                if (((g_Player.positionCenter.x < enemy->position.x) &&
                     (96.0 < enemy->position.x)) ||
                    (288.0 < enemy->position.x))
                {
                    *GET_FLOAT_PTR(enemy, 0) =
                        utils::AddNormalizeAngle(
                            g_Rng.GetRandomFloatInRange(1.5707964f) + 2.3561945f, 0.0f);
                }
                else
                {
                    *GET_FLOAT_PTR(enemy, 0) =
                        g_Rng.GetRandomFloatInRange(1.5707964f) - 0.7853982f;
                }
                break;
            case 0xa0:
                g_GameManager.AddCherryPlus(GET_INT_VALUE(enemy, 0));
                break;
            case 0xa1:
                enemy->flags4 = (enemy->flags4 & 0xf7) |
                                ((u8)GET_INT_VALUE(enemy, 0) & 1) << 3;
                break;
            }
            }
        }
        else
        {
            break;
        }
    skip:
        instr = (EclRawInstr *)((u8 *)instr + instr->size);
    }
    if ((enemy->flags1 & 3) == 1)
    {
        enemy->angle = utils::AddNormalizeAngle(
            enemy->angle,
            g_Supervisor.effectiveFramerateMultiplier * enemy->angularVelocity);
        enemy->moveSpeed = g_Supervisor.effectiveFramerateMultiplier *
                               enemy->moveAcceleration +
                           enemy->moveSpeed;
        AngleToVector(&enemy->axisSpeed, enemy->angle, enemy->moveSpeed);
        enemy->axisSpeed.z = 0.0f;
        if ((0 < enemy->moveInterpStartTime) &&
            (enemy->moveInterpTimer--,
             enemy->moveInterpTimer < 1))
        {
            enemy->flags1 = enemy->flags1 & 0xfc;
        }
    }
    else if ((enemy->flags1 & 3) == 2)
    {
        enemy->moveInterpTimer--;
        local_e8 = 1.0f - ((f32)enemy->moveInterpTimer.current +
                           enemy->moveInterpTimer.subFrame) /
                              (f32)enemy->moveInterpStartTime;
        if (local_e8 < 0.0f)
        {
            local_e8 = 0.0f;
        }
        switch (enemy->flags1 >> 2 & 7)
        {
        case 1: {
            local_e8 = local_e8 * local_e8;
            break;
        }
        case 2: {
            local_e8 = local_e8 * local_e8 * local_e8;
            break;
        }
        case 3: {
            local_e8 = local_e8 * local_e8 * local_e8 * local_e8;
            break;
        }
        case 4: {
            local_e8 = 1.0f - (1.0f - local_e8) * (1.0f - local_e8);
            break;
        }
        case 5: {
            local_e8 = 1.0f - local_e8;
            local_e8 = 1.0f - local_e8 * local_e8 * local_e8;
            break;
        }
        case 6: {
            local_e8 = 1.0f - local_e8;
            local_e8 = 1.0f - local_e8 * local_e8 * local_e8 * local_e8;
        }
        }
        enemy->axisSpeed =
            (local_e8 * enemy->moveInterp + enemy->moveInterpStartPos) -
            enemy->position;
        if ((enemy->flags1 >> 6 & 1) != 0)
        {
            enemy->axisSpeed.x = -enemy->axisSpeed.x;
        }
        enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
        if (enemy->moveInterpTimer < 1)
        {
            enemy->flags1 = enemy->flags1 & 0xfc;
            enemy->position = enemy->moveInterpStartPos + enemy->moveInterp;
            enemy->axisSpeed = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        }
    }
    else if ((enemy->flags1 & 3) == 3)
    {
        enemy->moveAngle = utils::AddNormalizeAngle(
            enemy->moveAngle, g_Supervisor.effectiveFramerateMultiplier *
                                  enemy->moveAngularVelocity);
        enemy->moveRadius = g_Supervisor.effectiveFramerateMultiplier *
                                enemy->moveRadialVelocity +
                            enemy->moveRadius;
        AngleToVector(&local_d8, enemy->moveAngle, enemy->moveRadius);
        enemy->axisSpeed.x =
            (local_d8.x + enemy->moveInterpStartPos.x) - enemy->position.x;
        enemy->axisSpeed.y =
            (local_d8.y + enemy->moveInterpStartPos.y) - enemy->position.y;
        enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
        if ((0 < enemy->moveInterpStartTime) &&
            (enemy->moveInterpTimer--,
             enemy->moveInterpTimer < 1))
        {
            enemy->flags1 &= 0xfc;
        }
    }
    if (0 < enemy->life)
    {
        if (0 < enemy->shootInterval)
        {
            enemy->shootIntervalTimer++;
            if (enemy->shootInterval <= enemy->shootIntervalTimer.current)
            {
                enemy->bulletProps.position = enemy->position + enemy->shootOffset;
                g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
                enemy->shootIntervalTimer = 0;
            }
        }
        if (enemy->anmExLeft >= 0)
        {
            uVar6 = 0;
            if ((enemy->flags1 >> 6 & 1) == 0)
            {
                if (-0.01 <= enemy->axisSpeed.x)
                {
                    if (0.01 < enemy->axisSpeed.x)
                    {
                        uVar6 = 2;
                    }
                }
                else
                {
                    uVar6 = 1;
                }
            }
            else if (-0.01 <= enemy->axisSpeed.x)
            {
                if (0.01 < enemy->axisSpeed.x)
                {
                    uVar6 = 1;
                }
            }
            else
            {
                uVar6 = 2;
            }
            if (enemy->anmExFlags != uVar6)
            {
                if (uVar6 == 0)
                {
                    if (enemy->anmExFlags == 0xff)
                    {
                        g_AnmManager->SetAnmIdxAndExecuteScript(
                            &enemy->primaryVm,
                            enemy->anmExDefaults + 0x900);
                    }
                    else if (enemy->anmExFlags == 1)
                    {
                        g_AnmManager->SetAnmIdxAndExecuteScript(
                            &enemy->primaryVm,
                            enemy->anmExFarLeft + 0x900);
                    }
                    else
                    {
                        g_AnmManager->SetAnmIdxAndExecuteScript(
                            &enemy->primaryVm,
                            enemy->anmExFarRight + 0x900);
                    }
                }
                else if (uVar6 == 1)
                {
                    g_AnmManager->SetAnmIdxAndExecuteScript(
                        &enemy->primaryVm,
                        enemy->anmExLeft + 0x900);
                }
                else if (uVar6 == 2)
                {
                    g_AnmManager->SetAnmIdxAndExecuteScript(
                        &enemy->primaryVm,
                        enemy->anmExRight + 0x900);
                }
                enemy->anmExFlags = uVar6;
            }
        }
        if (enemy->currentContext.func != NULL)
        {
            enemy->currentContext.func(enemy, enemy->currentContext.eclExInstr);
        }
        bVar7 = false;
        local_fc = enemy->currentContext.interps;
        fVar8 = enemy->position.x;
        fVar28 = enemy->position.y;
        fVar3 = enemy->position.z;
        for (local_f0 = 0; local_f0 < 8; local_f0 += 1)
        {
            if (local_fc->fn != NULL)
            {
                local_fc->timer++;
                if (local_fc->args[0].i <= local_fc->timer.current)
                {
                    local_fc->timer = local_fc->args[0].i;
                }
                local_f4 =
                    ((f32)local_fc->timer.current + local_fc->timer.subFrame) /
                    (f32)local_fc->args[0].i;
                switch (local_fc->args[2].i)
                {
                case 1: {
                    local_f4 = local_f4 * local_f4;
                    break;
                }
                case 2: {
                    local_f4 = local_f4 * local_f4 * local_f4;
                    break;
                }
                case 3: {
                    local_f4 = local_f4 * local_f4 * local_f4 * local_f4;
                    break;
                }
                case 4: {
                    local_f4 = 1.0f - (1.0f - local_f4) * (1.0f - local_f4);
                    break;
                }
                case 5: {
                    local_f4 = 1.0f - local_f4;
                    local_f4 = 1.0f - local_f4 * local_f4 * local_f4;
                    break;
                }
                case 6: {
                    local_f4 = 1.0f - local_f4;
                    local_f4 = 1.0f - local_f4 * local_f4 * local_f4 * local_f4;
                }
                }
                local_fc->fn(enemy, local_fc, local_f4);
                if (local_fc->args[0].i <= local_fc->timer.current)
                {
                    local_fc->fn = NULL;
                }
                if (((local_fc->args[7].f == 10018.0f) ||
                     (local_fc->args[7].f == 10019.0f)) ||
                    (local_fc->args[7].f == 10020.0f))
                {
                    bVar7 = true;
                }
            }
            local_fc = local_fc + 1;
        }
        if (bVar7)
        {
            enemy->axisSpeed.x = enemy->position.x - fVar8;
            enemy->axisSpeed.y = enemy->position.y - fVar28;
            enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
            enemy->position.x = fVar8;
            enemy->position.y = fVar28;
            enemy->position.z = fVar3;
        }
    }
    enemy->currentContext.curInstr = instr;
    enemy->currentContext.time++;
    if ((enemy->isBoss && (enemy->bossId == 0)) &&
        (g_EnemyManager.spellcardInfo.isActive != 0 &&
         (g_EnemyManager.spellcardInfo.isCapturing != 0)))
    {
        if (!enemy->isSurvivalSpellcard)
        {
            uVar17 =
                (f32)(i32)
                    g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx] -
                (((f32)g_EnemyManager.timer.current +
                  g_EnemyManager.timer.subFrame) *
                 (f32)g_EnemyManager.spellcardInfo.scoreDrainRate) /
                    60.0;
            g_EnemyManager.spellcardInfo.captureScore =
                (i32)((f32)(i32)
                          g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx] -
                      (((f32)g_EnemyManager.timer.current +
                        g_EnemyManager.timer.subFrame) *
                       (f32)g_EnemyManager.spellcardInfo.scoreDrainRate) /
                          60.0f);
            g_EnemyManager.spellcardInfo.captureScore = g_EnemyManager.spellcardInfo.captureScore - (g_EnemyManager.spellcardInfo.captureScore % 10);
        }
        g_EnemyManager.timer++;
    }
    if (enemy->isBoss && (6 < g_GameManager.currentStage))
    {
        if ((g_Player.bombInfo.isInUse == 0) ||
            (g_EnemyManager.spellcardInfo.isActive == 0 ||
             ((i32)g_EnemyManager.spellcardInfo.spellcardIdx < 0x76)))
        {
            if (enemy->spellcardDelayTimer == 0)
            {
                enemy->flags4 = enemy->flags4 & 0xfb;
            }
            else
            {
                enemy->spellcardDelayTimer = enemy->spellcardDelayTimer + -1;
            }
        }
        else
        {
            enemy->flags4 = enemy->flags4 | 4;
            enemy->spellcardDelayTimer = 1;
        }
    }
    return ZUN_SUCCESS;
}
