#include "pch.h"
#include "dsutil.h"
#include "TMFieldScene.h"
#include "TMGlobal.h"
#include "TMLog.h"
#include "SControlContainer.h"
#include "SGrid.h"
#include "Mission.h"
#include "MrItemMix.h"
#include "TMGround.h"
#include "TMHuman.h"
#include "TMObjectContainer.h"
#include "TMCamera.h"
#include "TMSun.h"
#include "TMSky.h"
#include "TMSnow.h"
#include "TMRain.h"
#include "TMSkinMesh.h"
#include "TMEffectMesh.h"
#include "TMEffectBillBoard2.h"
#include "TMEffectBillBoard.h"
#include "TMUtil.h"
#include "TMHouse.h"
#include "TMEffectBillBoard4.h"
#include "TMSkillMagicArrow.h"
#include "TMEffectStart.h"
#include "TMSkillJudgement.h"
#include "TMSkillTownPortal.h"
#include "TMEffectLevelUp.h"
#include "TMEffectSkinMesh.h"
#include "SGrid.h"
#include "ItemEffect.h"
#include "TMUtil.h"
#include "TMEffectSWSwing.h"
#include "TMEffectSpark.h"
#include "TMSkillHolyTouch.h"
#include "TMEffectParticle.h"
#include "TMSkillMeteorStorm.h"
#include "TMSkillThunderBolt.h"
#include "TMSkillSlowSlash.h"
#include "TMSkillMagicShield.h"
#include "TMSkillFreezeBlade.h"
#include "TMArrow.h"
#include "TMShade.h"
#include "TMSkillPoison.h"
#include "TMFont3.h"
#include "TMSkillHeavenDust.h"
#include "TMSkillFlash.h"
#include "TMItem.h"
#include "TMCannon.h"
#include "TMEffectCharge.h"
#include "TMSkillExplosion2.h"
#include "TMEffectDust.h"
#include "TMGate.h"
#include <WinInet.h>
#include <regex>
#include <windows.h>
#include <iostream>
#include <io.h>
#include <thread>
#include <chrono>

// ========== Variáveis globais (originais) ==========
bool MacroUP = false;
bool MacroPesa = false;
int PossX;
int PossY;

bool AguaMacroN = false;
bool AguaMacroM = false;
bool AguaMacroA = false;

int needupdate = 0;

int countAguaN = 0;
int countAguaM = 0;
int countAguaA = 0;
int AutoTradeNew = 1;

// ========== Sincronização automática ==========
struct AutoCriticalSection {
    CRITICAL_SECTION cs;
    AutoCriticalSection() {
        InitializeCriticalSection(&cs);
    }
    ~AutoCriticalSection() {
        DeleteCriticalSection(&cs);
    }
} g_autoCS;

#define g_csMacro g_autoCS.cs

HANDLE g_hMacroThread = NULL;

// ========== Função original com sincronização ==========
void TMFieldScene::FiltroPosition()
{
    EnterCriticalSection(&g_csMacro);
    if (m_pMyHuman)
    {
        PossX = (int)m_pMyHuman->m_vecPosition.x;
        PossY = (int)m_pMyHuman->m_vecPosition.y;
    }
    LeaveCriticalSection(&g_csMacro);
}

// ========== Mapeamento de salas → nível mínimo ==========
struct SalaInfo {
    int x, y;
    int minLevel;  // 1-8, 9 = Boss
};

SalaInfo salas[] = {
    {1250, 3682, 1},
    {1214, 3682, 2},
    {1178, 3682, 3},
    {1178, 3646, 4},
    {1178, 3610, 5},
    {1214, 3610, 6},
    {1250, 3610, 7},
    {1250, 3646, 8},
    {1214, 3646, 9}  // Boss
};

int GetMinLevel(int Xm, int Ym) {
    // Área da água: nível mínimo 1
    if ((Xm > 1954 && Ym > 1760 && Xm < 1976 && Ym < 1776) ||
        (Xm >= 1956 && Xm <= 1975 && Ym >= 1766 && Ym <= 1775) ||
        (Xm >= 1955 && Xm <= 1976 && Ym >= 1764 && Ym <= 1780))
        return 1;

    // Salas
    for (int i = 0; i < 9; i++) {
        if (abs(Xm - salas[i].x) <= 5 && abs(Ym - salas[i].y) <= 5)
            return salas[i].minLevel;
    }
    return -1; // inválido
}

// ========== Prioridade de pergaminhos ==========
int FindBestScroll(STRUCT_MOB* mob, int minLv) {
    // Lista de prioridade (ID base M)
    int priority[] = {
        785, // Boss
        784, // Lv8
        783, // Lv7
        782, // Lv6
        781, // Lv5
        780, // Lv4
        779, // Lv3
        778, // Lv2
        777  // Lv1
    };

    for (int p = 0; p < 9; p++) {
        int desired = priority[p];
        // converte nível para minLv: Boss = 9, Lv8 = 8, etc.
        int itemLv = (desired == 785) ? 9 : desired - 776;

        if (itemLv < minLv) continue; // nível insuficiente

        for (int i = 0; i < MAX_CARRY; i++) {
            int item = mob->Carry[i].sIndex;
            if (item == 0) continue;

            // Série M
            if (item == desired) return i;

            // Série N (3173-3181 correspondem a 777-785)
            if (item >= 3173 && item <= 3181) {
                int converted = 777 + (item - 3173);
                if (converted == desired) return i;
            }

            // Série A (3182-3190 correspondem a 777-785)
            if (item >= 3182 && item <= 3190) {
                int converted = 777 + (item - 3182);
                if (converted == desired) return i;
            }
        }
    }
    return -1;
}

// ========== Comando do chat ==========
int TMFieldScene::ComandosChat(char* text)
{
    if (!strcmp(text, "autowater"))
    {
        EnterCriticalSection(&g_csMacro);
        MacroUP = !MacroUP;
        bool macroState = MacroUP;
        if (!MacroUP)
        {
            AguaMacroN = false;
            AguaMacroM = false;
            AguaMacroA = false;
        }
        LeaveCriticalSection(&g_csMacro);

        char str[128] = {};
        sprintf(str, "AutoWater : %s", (macroState ? "ON" : "OFF"));
        m_pMessagePanel->SetMessage(strFmt(str), 4000);
        m_pMessagePanel->SetVisible(1, 2);

        if (macroState)
        {
            if (g_hMacroThread == NULL)
            {
                DWORD threadID;
                g_hMacroThread = CreateThread(NULL, 0, ThreadMacro, NULL, 0, &threadID);
                if (!g_hMacroThread)
                {
                    LOG_WRITELOG("[ERROR] Failed to create MacroPerga thread\r\n");
                    EnterCriticalSection(&g_csMacro);
                    MacroUP = false;
                    LeaveCriticalSection(&g_csMacro);
                }
            }
        }
        else
        {
            if (g_hMacroThread)
            {
                WaitForSingleObject(g_hMacroThread, 5000);
                CloseHandle(g_hMacroThread);
                g_hMacroThread = NULL;
            }
        }
        return TRUE;
    }
    return FALSE;
}

// ========== Thread principal ==========
DWORD WINAPI ThreadMacro(LPVOID lpParameter)
{
    int lastItem = 0;
    DWORD lastTime = 0;

    while (true)
    {
        EnterCriticalSection(&g_csMacro);
        if (!MacroUP)
        {
            LeaveCriticalSection(&g_csMacro);
            break;
        }
        int localX = PossX;
        int localY = PossY;
        LeaveCriticalSection(&g_csMacro);

        if (localX == 0 && localY == 0)
        {
            Sleep(1000);
            continue;
        }

        int minLv = GetMinLevel(localX, localY);
        if (minLv == -1)
        {
            Sleep(3000);
            continue;
        }

        // Acesso ao inventário
        EnterCriticalSection(&g_csMacro);
        STRUCT_MOB* mob = &g_pObjectManager->m_stMobData;
        int slot = FindBestScroll(mob, minLv);
        LeaveCriticalSection(&g_csMacro);

        if (slot == -1)
        {
            Sleep(3000);
            continue;
        }

        int currentItem = g_pObjectManager->m_stMobData.Carry[slot].sIndex;

        // Anti-spam: não reutiliza o mesmo item em menos de 10s
        if (currentItem == lastItem && GetTickCount() - lastTime < 10000)
        {
            Sleep(500);
            continue;
        }

        MSG_UseItem p;
        memset(&p, 0, sizeof(p));
        p.Header.Type = 0x373;
        p.Header.Size = sizeof(p);
        p.Header.ID = g_pObjectManager->m_dwCharID;
        p.GridX = localX;
        p.GridY = localY;
        p.SourType = 1;
        p.SourPos = slot;

        EnterCriticalSection(&g_csMacro);
        SendOneMessage((char*)&p, sizeof(p));
        LeaveCriticalSection(&g_csMacro);

        LOG_WRITELOG("[AutoWater] Item=%d Slot=%d MinLv=%d (%d,%d)\r\n",
            currentItem, slot, minLv, localX, localY);

        lastItem = currentItem;
        lastTime = GetTickCount();

        Sleep(1000);
    }
    return 0;
}