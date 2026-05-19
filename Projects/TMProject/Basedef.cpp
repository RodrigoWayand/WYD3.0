#include "pch.h"
#include "Basedef.h"
#include "TMGlobal.h"
#include "TMLog.h"
#include "ItemEffect.h"
#include <WinInet.h>
#include <WinSock2.h>

char g_pAffectTable[MAX_EFFECT_STRING_TABLE][24];
char g_pAffectSubTable[MAX_SUB_EFFECT_STRING_TABLE][24];
int  g_pHitRate[1024];

HWND hWndMain;
char EncodeByte[4];
int  g_nChannelWidth;
int  g_nServerGroupNum;
char g_pServerList[MAX_SERVERGROUP][MAX_SERVERNUMBER][64];
int  g_nSelServerWeather;
char g_pMessageStringTable[MAX_STRING][MAX_STRING_LENGTH];

STRUCT_ITEMLIST g_pItemList[MAX_ITEMLIST];
STRUCT_SPELL    g_pSpell[MAX_SPELL_LIST];
STRUCT_INITITEM g_pInitItem[100];
int             g_itemicon[6500];

STRUCT_GUILDZONE g_pGuildZone[MAX_GUILDZONE] =
{
    {0, 0, 2088, 2148, 2086, 2093, 2052, 2052, 2171, 2163, 197, 213, 238, 230, 205, 220, 228, 220, 5, 0}, // Armia
    {0, 0, 2531, 1700, 2494, 1707, 2432, 1672, 2675, 1767, 197, 149, 238, 166, 205, 157, 228, 157, 5, 0}, // Azran
    {0, 0, 2460, 1976, 2453, 2000, 2448, 1966, 2476, 2024, 141, 213, 182, 230, 146, 220, 173, 220, 5, 0}, // Erion
    {0, 0, 3614, 3124, 3652, 3122, 3605, 3090, 3690, 3260, 141, 149, 182, 166, 146, 157, 173, 157, 5, 0}, // Nippleheim
    {0, 0, 1066, 1760, 1050, 1706, 1036, 1700, 1072, 1760, 4000, 4000, 4010, 4010, 4005, 4005, 4005, 4005, 5, 0} // Noatum
};

// ---------------------------------------------------------------------------
// Screen / module helpers
// ---------------------------------------------------------------------------

float BASE_ScreenResize(float size)
{
    float sc = (float)g_pDevice->m_dwScreenWidth * (size / 1366.0f);

    const DWORD w = g_pDevice->m_dwScreenWidth;
    if (w == 800 || w == 640 || w == 1024 || w == 1280)
        sc = (float)w * (size / 800.0f);

    return sc;
}

void BASE_InitModuleDir()
{
    char path[256]{};
    GetModuleFileName(nullptr, path, sizeof(path));

    for (int i = (int)strlen(path) - 1; i > 0; --i)
    {
        if (path[i] == '\\')
        {
            path[i] = '\0';
            break;
        }
    }

    SetCurrentDirectory(path);
}

// ---------------------------------------------------------------------------
// Hit-rate table initialisation
// ---------------------------------------------------------------------------

void BASE_InitializeHitRate()
{
    memset(g_pHitRate, 0, sizeof(g_pHitRate));

    int jump = 512;
    int start = 0;
    int quad = 0;

    do
    {
        for (int i = 0; i < 1024; i += jump)
        {
            if (g_pHitRate[i])
                continue;

            switch (quad)
            {
            case 0: g_pHitRate[i] = start;         break;
            case 1: g_pHitRate[i] = 512 - start;   break;
            case 2: g_pHitRate[i] = start + 512;   break;
            default: g_pHitRate[i] = 1024 - start; break;
            }

            if (g_pHitRate[i] > 999)
                g_pHitRate[i] = 999;

            if (++quad >= 4)
            {
                quad = 0;
                ++start;
            }
        }

        jump /= 2;
    } while (jump);

    g_pHitRate[0] = 512;
}

// ---------------------------------------------------------------------------
// Attribute map
// ---------------------------------------------------------------------------

int BASE_InitializeAttribute()
{
    FILE* fp = nullptr;
    fopen_s(&fp, "./Env/AttributeMap.dat", "rb");
    if (!fp)
        fopen_s(&fp, "../../TMSRV/Run/AttributeMap.dat", "rb");

    if (!fp)
    {
        MessageBox(0, "There is no file", "Attributemap.dat", MB_OK);
        return 0;
    }

    fread(g_pAttribute, 1024, 1024, fp);

    int tsum = 0;
    fread(&tsum, 4, 1, fp);
    fclose(fp);

    BASE_GetSum((char*)g_pAttribute, sizeof(g_pAttribute));
    return true;
}

void BASE_ApplyAttribute(char* pHeight, int size)
{
    const int endx = size + g_HeightPosX;
    const int endy = size + g_HeightPosY;

    for (int y = g_HeightPosY; y < endy; ++y)
    {
        for (int x = g_HeightPosX; x < endx; ++x)
        {
            if (g_pAttribute[(y >> 2) & 0x3FF][(x >> 2) & 0x3FF] & 2)
                pHeight[x + g_HeightWidth * (y - g_HeightPosY) - g_HeightPosX] = 127;
        }
    }
}

// ---------------------------------------------------------------------------
// Binary data loaders
// ---------------------------------------------------------------------------

int BASE_ReadItemList()
{
    FILE* fp = nullptr;
    fopen_s(&fp, ".\\ItemList.bin", "rb");

    if (!fp)
    {
        MessageBoxA(0, "Can't read ItemList.bin", "ERROR", 0);
        return 0;
    }

    const int size = sizeof(g_pItemList);
    char* temp = (char*)g_pItemList;
    int   tsum = 0;

    fread(g_pItemList, size, 1u, fp);
    fread(&tsum, 4u, 1u, fp);
    fclose(fp);

#if !defined(_DEBUG)
    if (tsum != 0x1343B16)
        return 0;
#endif

    for (int i = 0; i < size; ++i)
        temp[i] ^= 0x5A;

    int handle = _open(".\\ExtraItem.bin", _O_RDONLY | _O_BINARY, 0);

    if (handle != -1)
    {
        char buff[256]{};

        while (_read(handle, buff, sizeof(STRUCT_ITEMLIST) + 2) >= (int)(sizeof(STRUCT_ITEMLIST) + 2))
        {
            short idx = *(short*)buff;

            if (idx > 0 && idx < MAX_ITEMLIST)
                memcpy(&g_pItemList[idx], &buff[2], sizeof(STRUCT_ITEMLIST));
        }

        _close(handle);

        for (int j = 0; j < size; ++j)
            temp[j] ^= 0x5A;

#if !defined(_DEBUG)
        if (tsum != 0x1343B16)
            return 0;
#endif

        // Adicionando linha em branco após #endif para separar do for loop e melhorar legibilidade

        for (int j = 0; j < size; ++j)
            temp[j] ^= 0x5A;
    }

    return 1;
}

int BASE_ReadSkillBin()
{
    const int size = sizeof(STRUCT_SPELL) * MAX_SPELL_LIST;
    char* temp = (char*)g_pSpell;
    int   tsum = 0;

    FILE* fp = fopen(SkillData_Path, "rb");

    if (!fp)
    {
        MessageBox(NULL, "Can't read SkillData.bin", "ERROR", NULL);
        return FALSE;
    }

    fread(g_pSpell, size, 1, fp);
    fread(&tsum, sizeof(tsum), 1, fp);
    fclose(fp);

    BASE_GetSum2((char*)g_pSpell, size);

    for (int i = 0; i < size; ++i)
        temp[i] ^= 0x5A;

    return TRUE;
}

int BASE_ReadInitItem()
{
    // Check if is really necessarily
    return 1;
}

// ---------------------------------------------------------------------------
// Item price overrides
// ---------------------------------------------------------------------------

void BASE_InitialItemRePrice()
{
    g_pItemList[412].nPrice = 4000000;
    g_pItemList[413].nPrice = 8000000;
    g_pItemList[419].nPrice = 400000;
    g_pItemList[420].nPrice = 800000;
}

// ---------------------------------------------------------------------------
// Checksum helpers
// NOTE: The loose `else` in both functions is intentional — it accumulates
// p[i]/2 or p[i]/3 on most iterations alongside the specific-mod branches.
// Changing this would break validation against existing binary files.
// ---------------------------------------------------------------------------

int BASE_GetSum(char* p, int size)
{
    int sum = 0;

    for (int i = 0; i < size; ++i)
    {
        const int mod = i % 7;

        if (!mod)       sum += p[i] / 2;
        if (mod == 1)   sum += p[i] ^ 0xFF;
        if (mod == 2)   sum += 3 * p[i];
        if (mod == 3)   sum += 2 * p[i];
        if (mod == 4)   sum -= p[i] / 7;
        if (mod == 5)   sum -= p[i];
        else            sum += p[i] / 3;
    }

    return sum;
}

int BASE_GetSum2(char* p, int size)
{
    int sum = 0;

    for (int i = 0; i < size; ++i)
    {
        const int mod = i % 9;

        if (mod == 0) sum += 2 * p[i];
        if (mod == 1) sum += p[i] ^ 0xFF;
        if (mod == 2) sum += p[i] / 3;
        if (mod == 3) sum += 2 * p[i];
        if (mod == 4) sum -= p[i] ^ 0x5A;
        if (mod == 5) sum -= p[i];
        else          sum += p[i] / 5;
    }

    return sum;
}

// ---------------------------------------------------------------------------
// String / message tables
// ---------------------------------------------------------------------------

int BASE_ReadMessageBin()
{
    memset(g_pMessageStringTable, 0, sizeof(g_pMessageStringTable));

    const int size = 256000;
    FILE* pFile = nullptr;
    fopen_s(&pFile, Strdef_Path, "rb");

    int checksum = 0;

    if (pFile)
    {
        fread(g_pMessageStringTable, size, 1u, pFile);
        fread(&checksum, 4, 1, pFile);
        fclose(pFile);
    }

    if (checksum != BASE_GetSum((char*)g_pMessageStringTable, size))
        return 0;

    for (int i = 0; i < 2000; ++i)
        for (int k = 0; k < 128; ++k)
            g_pMessageStringTable[i][k] ^= 0x5A;

    return 1;
}

void BASE_InitEffectString()
{
    FILE* fpEffect = nullptr;
    fopen_s(&fpEffect, EffectString_Path, "rt");

    if (fpEffect)
    {
        for (int i = 1; i < MAX_EFFECT_STRING_TABLE; ++i)
            fscanf(fpEffect, "%24s", &g_pAffectTable[i][0]);

        fclose(fpEffect);
    }

    FILE* fpEffectSub = nullptr;
    fopen_s(&fpEffectSub, EffectSubString_Path, "rt");

    if (fpEffectSub)
    {
        for (int j = 0; j < MAX_SUB_EFFECT_STRING_TABLE; ++j)
            fscanf(fpEffectSub, "%s", &g_pAffectSubTable[j][0]);

        fclose(fpEffectSub);
    }

    /* There's a loading of the GuildString.txt file, but is not used */
}

// ---------------------------------------------------------------------------
// Initialisation entry-point
// ---------------------------------------------------------------------------

int BASE_InitializeBaseDef()
{
    int ret = 0;
    ret = BASE_InitializeServerList() & 1;
    ret = BASE_ReadSkillBin() & ret;
    ret = BASE_ReadItemList() & ret;
    ret = BASE_ReadInitItem() & ret;
    ret = BASE_InitializeAttribute() & ret;

    BASE_InitialItemRePrice();

    return ret;
}

void BASE_ReadItemPrice()
{
    int itemprice[MAX_ITEM_PRICE_REPLACE][2]{};

    FILE* fp = nullptr;
    fopen_s(&fp, ItemPrice_Path, "rb");

    if (fp)
    {
        fread(itemprice, sizeof(itemprice), 1, fp);
        fclose(fp);
    }

    for (int k = 0; k < MAX_ITEM_PRICE_REPLACE && itemprice[k][0]; ++k)
    {
        const int idx = itemprice[k][0];
        g_pItemList[idx].nPrice = itemprice[k][1];
    }
}

// ---------------------------------------------------------------------------
// String utilities
// ---------------------------------------------------------------------------

void BASE_UnderBarToSpace(char* szStr)
{
    while (*szStr)
    {
        if (*szStr == '_')
            *szStr = ' ';

        ++szStr;
    }
}

// ---------------------------------------------------------------------------
// HTTP helper
// ---------------------------------------------------------------------------

int BASE_GetHttpRequest(char* httpname, char* Request, int MaxBuffer)
{
    auto hSession = InternetOpen("MS", 0, 0, 0, 0);
    if (!hSession)
        return 0;

    auto hHttpFile = InternetOpenUrl(hSession, httpname, 0, 0, 0x4000000u, 0);

    if (!hHttpFile)
    {
        GetLastError();
        InternetCloseHandle(hSession);
        return 0;
    }

    DWORD dwBytesRead = 0;
    InternetReadFile(hHttpFile, Request, MaxBuffer, &dwBytesRead);
    InternetCloseHandle(hHttpFile);

    if (dwBytesRead >= 1024)
        dwBytesRead = 1023;

    Request[dwBytesRead] = '\0';
    InternetCloseHandle(hSession);

    return 1;
}

// ---------------------------------------------------------------------------
// Time utilities
// BUG FIX: week agora usa 7 dias em segundos (86400 * 7) em vez de 1 dia
// ---------------------------------------------------------------------------

int BASE_GetWeekNumber()
{
    time_t now;
    time(&now);

    const unsigned int week = 86400 * 7;
    return (int)(now / week - 3);
}

// ---------------------------------------------------------------------------
// Item sanctification helpers
// ---------------------------------------------------------------------------

int BASE_GetItemSanc(STRUCT_ITEM* item)
{
    const int idx = item->sIndex;

    if ((idx >= 2330 && idx < 2390) ||
        (idx >= 3200 && idx < 3300) ||
        (idx >= 3980 && idx < 4000))
    {
        return 0;
    }

    int sanc = 0;

    const bool noDirectSanc = (item->stEffect[0].cEffect != EF_SANC &&
        item->stEffect[1].cEffect != EF_SANC &&
        item->stEffect[2].cEffect != EF_SANC);

    if (noDirectSanc)
    {
        for (int i = 0; i < 3; ++i)
        {
            const auto eff = item->stEffect[i].cEffect;
            if (eff >= 115 && eff <= 126)
            {
                sanc = item->stEffect[i].cValue;
                break;
            }
        }
    }
    else
    {
        if (item->stEffect[0].cEffect == EF_SANC)
            sanc = item->stEffect[0].cValue;
        else if (item->stEffect[1].cEffect == EF_SANC)
            sanc = item->stEffect[1].cValue;
        else
            sanc = item->stEffect[2].cValue;
    }

    if (idx != 786 && idx != 1936 && idx != 1937)
    {
        if (sanc < 230)
            sanc %= 10;
        else
            sanc -= 220;

        if (sanc >= 10 && sanc <= 35)
            sanc = (sanc - 10) / 4 + 10;
    }

    return sanc;
}

// ---------------------------------------------------------------------------
// Item ability calculators
// ---------------------------------------------------------------------------

int BASE_GetItemAbility(STRUCT_ITEM* item, char Type)
{
    int value = 0;
    const int idx = item->sIndex;

    if (idx <= 0 || idx > MAX_ITEMLIST)
        return 0;

    const int nUnique = g_pItemList[idx].nUnique;
    const int nPos = g_pItemList[idx].nPos;

    if ((Type == EF_DAMAGEADD || Type == EF_MAGICADD) && (nUnique < 41 || nUnique > 50))
        return 0;

    if (Type == EF_CRITICAL &&
        (item->stEffect[1].cEffect == EF_CRITICAL2 || item->stEffect[2].cEffect == EF_CRITICAL2))
    {
        Type = EF_CRITICAL2;
    }

    if (Type == EF_DAMAGE && nPos == 32 &&
        (item->stEffect[1].cEffect == EF_DAMAGE2 || item->stEffect[2].cEffect == EF_DAMAGE2))
    {
        Type = EF_DAMAGE2;
    }

    if (Type == EF_MPADD &&
        (item->stEffect[1].cEffect == EF_MPADD2 || item->stEffect[2].cEffect == EF_MPADD2))
    {
        Type = EF_MPADD2;
    }

    if (Type == EF_HPADD &&
        (item->stEffect[1].cEffect == EF_HPADD2 || item->stEffect[2].cEffect == EF_HPADD2))
    {
        Type = EF_HPADD2;
    }

    if (Type == EF_ACADD &&
        (item->stEffect[1].cEffect == EF_ACADD2 || item->stEffect[2].cEffect == EF_ACADD2))
    {
        Type = EF_ACADD2;
    }

    if (Type == EF_LEVEL)    value = g_pItemList[idx].nReqLvl;
    if (Type == EF_REQ_STR)  value += g_pItemList[idx].nReqStr;
    if (Type == EF_REQ_INT)  value += g_pItemList[idx].nReqInt;
    if (Type == EF_REQ_DEX)  value += g_pItemList[idx].nReqDex;
    if (Type == EF_REQ_CON)  value += g_pItemList[idx].nReqCon;

    if (Type == EF_POS)
    {
        value += g_pItemList[idx].nPos;
        if (g_pItemList[idx].nPos == -32768)
            value = 32768;
    }

    if (Type != EF_INCUBATE)
    {
        for (int i = 0; i < 12; ++i)
        {
            if (g_pItemList[idx].stEffect[i].sEffect == Type ||
                (g_pItemList[idx].stEffect[i].sEffect == EF_HPADD && Type == EF_HPADD2))
            {
                int tvalue = g_pItemList[idx].stEffect[i].sValue;
                if (Type == EF_ATTSPEED && tvalue == 1)
                    tvalue = 10;
                value += tvalue;
            }
        }
    }

    // Mount items (2330-2389)
    if (idx >= 2330 && idx < 2390)
    {
        switch (Type)
        {
        case EF_MOUNTHP:   return item->stEffect[0].sValue;
        case EF_MOUNTSANC: return item->stEffect[1].cEffect;
        case EF_MOUNTLIFE: return item->stEffect[1].cValue;
        case EF_MOUNTFEED: return item->stEffect[2].cEffect;
        case EF_MOUNTKILL: return item->stEffect[2].cValue;
        }

        if (idx < 2362 || idx >= 2390 || item->stEffect[0].sValue <= 0)
            return value;

        const int lv = item->stEffect[1].cEffect;
        const int cd = idx - 2360;

        switch (Type)
        {
        case EF_DAMAGE:    value = g_pMountBonus[cd][0] * (lv + 20) / 100; break;
        case EF_MAGIC:     value = g_pMountBonus[cd][1] * (lv + 15) / 100; break;
        case EF_PARRY:     value = g_pMountBonus[cd][2];                    break;
        case EF_RESISTALL: value = g_pMountBonus[cd][3];                    break;
        default: break;
        }
    }
    // Mount items 2 (3980-3999)
    else if (idx >= 3980 && idx < 4000)
    {
        const int cd = idx - 3980;

        switch (Type)
        {
        case EF_DAMAGE:    value = g_pMountBonus2[cd][0]; break;
        case EF_MAGIC:     value = g_pMountBonus2[cd][1]; break;
        case EF_PARRY:     value = g_pMountBonus2[cd][2]; break;
        case EF_RESISTALL: value = g_pMountBonus2[cd][3]; break;
        default: break;
        }
    }
    else
    {
        for (int j = 0; j < 3; ++j)
        {
            if (item->stEffect[j].cEffect == Type)
            {
                int tvalue = item->stEffect[j].cValue;
                if (Type == EF_ATTSPEED && tvalue == 1)
                    tvalue = 10;
                value += tvalue;
            }
        }

        int sanc = BASE_GetItemSanc(item);

        if (idx <= 40)
            sanc = 0;

        if (sanc >= 9 && (nPos & 0xF00))
            sanc++;

        const bool skipSanc =
            (Type == EF_GRID || Type == EF_CLASS || Type == EF_POS ||
                Type == EF_WTYPE || Type == EF_RANGE || Type == EF_LEVEL ||
                Type == EF_REQ_STR || Type == EF_REQ_INT || Type == EF_REQ_DEX ||
                Type == EF_REQ_CON || Type == EF_VOLATILE || Type == EF_INCUBATE ||
                Type == EF_INCUDELAY || Type == EF_PREVBONUS || Type == EF_TRANS ||
                Type == EF_REFLEVEL || Type == EF_GAMEROOM || Type == EF_REGENMP ||
                Type == EF_REGENHP || Type == EF_FAME);

        if (sanc && !skipSanc)
        {
            if (sanc > 10)
            {
                int UpSanc = sanc - 10;
                switch (UpSanc)
                {
                case 1: UpSanc = 220; break;
                case 2: UpSanc = 250; break;
                case 3: UpSanc = 280; break;
                case 4: UpSanc = 320; break;
                case 5: UpSanc = 370; break;
                case 6: UpSanc = 400; break;
                }
                value = UpSanc * 10 * value / 100 / 10;
            }
            else
            {
                value = value * (sanc + 10) / 10;
            }
        }

        if (Type == EF_RUNSPEED)
        {
            if (value >= 3)
                value = 2;
            if (value > 0 && sanc >= 9)
                value++;
        }

        if (Type == EF_HWORDGUILD || Type == EF_LWORDGUILD)
            value = value;

        if (Type == EF_REGENMP || Type == EF_REGENHP)
            value *= sanc;

        if (Type == EF_GRID && (value < 0 || value > 7))
            value = 0;
    }

    return value;
}

// ---------------------------------------------------------------------------
// Skin mesh / mount scale helpers
// ---------------------------------------------------------------------------

int BASE_DefineSkinMeshType(int nClass)
{
    switch (nClass)
    {
    case 1:  return 0;
    case 2:  return 1;
    case 4:  return 0;
    case 8:  return 1;
    case 16: return 20;
    case 17: return 21;
    case 18: return 22;
    case 19: return 23;
    case 20: return 24;
    case 21: return 2;
    case 22: return 25;
    case 23: return 26;
    case 24: return 27;
    case 25: return 2;
    case 26: return 3;
    case 27: return 28;
    case 28: return 29;
    case 29: return 6;
    case 30: return 4;
    case 31: return 32;
    case 32: return 7;
    case 33: return 8;
    case 34: return 0;
    case 35: return 29;
    case 36: return 0;
    case 37: return 1;
    case 38: return 1;
    case 39: return 0;
    case 40: return 0;
    case 41: return 69;
    case 42: return 30;
    case 43: return 31;
    case 44: return 33;
    case 45: return 23;
    case 46: return 11;
    case 47: return 35;
    case 48: return 34;
    case 49: return 36;
    case 50: return 37;
    case 51: return 38;
    case 52: return 39;
    case 53: return 40;
    case 54: return 9;
    case 55: return 10;
    case 56: return 41;
    case 57: return 12;
    case 58: return 42;
    case 59: return 43;
    case 60: return 0;
    case 61: return 1;
    case 62: return 5;
    case 63: return 0;
    case 64: return 44;
    case 66: return 45;
    case 67: return 46;
    case 68: return 47;
    case 69: return 48;
    case 70: return 53;
    case 71: return 54;
    case 72: return 55;
    case 73: return 56;
    case 74: return 57;
    }

    LOG_WRITELOG("Invalide Item Class %d\n", nClass);
    return 0;
}

float BASE_GetMountScale(int nSkinMeshType, int nMeshIndex)
{
    if (nSkinMeshType == 28)
        return 1.45f;
    if (nSkinMeshType == 25 && nMeshIndex == 1)
        return 1.4f;
    if (nSkinMeshType == 20 && nMeshIndex == 7)
        return 0.6f;
    if (nSkinMeshType == 20 && !nMeshIndex)
        return 1.3f;
    if (nSkinMeshType == 29 && nMeshIndex == 4)
        return 1.3f;

    return 1.0f;
}

// ---------------------------------------------------------------------------
// Path-finding / distance helpers
// ---------------------------------------------------------------------------

int BASE_GetRoute(int x, int y, int* targetx, int* targety, char* Route, int distance, char* pHeight, int MH)
{
    const int lastx = x;
    const int lasty = y;
    const int tx = *targetx;
    const int ty = *targety;
    memset(Route, 0, 24);

    for (int i = 0; i < distance && i < 23; ++i)
    {
        if (x - g_HeightPosX < 1 || y - g_HeightPosY < 1 ||
            x - g_HeightPosX > g_HeightWidth - 2 ||
            y - g_HeightPosY > g_HeightHeight - 2)
        {
            Route[i] = 0;
            break;
        }

        const int base_idx = x + g_HeightWidth * (y - g_HeightPosY) - g_HeightPosX;
        const int W = g_HeightWidth;

        const int cul = (signed char)pHeight[base_idx];
        const int n = (signed char)pHeight[base_idx - W];
        const int ne = (signed char)pHeight[base_idx - W + 1];
        const int e = (signed char)pHeight[base_idx + 1];
        const int se = (signed char)pHeight[base_idx + W + 1];
        const int s = (signed char)pHeight[base_idx + W];
        const int sw = (signed char)pHeight[base_idx + W - 1];
        const int w = (signed char)pHeight[base_idx - 1];
        const int nw = (signed char)pHeight[base_idx - W - 1];

        int nMH = MH;
        if (cul >= -1 && cul <= 1 &&
            (n < 0 || ne < 0 || e < 0 || se < 0 || s < 0 || sw < 0 || w < 0 || nw < 0))
        {
            nMH = 30;
        }

        if (tx == x && ty == y) { Route[i] = 0;  break; }

        if (tx == x && ty > y && s < nMH + cul && s > cul - nMH) { Route[i] = 56; ++y; }
        else if (tx == x && ty < y && n < nMH + cul && n > cul - nMH) { Route[i] = 50; --y; }
        else if (tx > x && ty < y && ne < nMH + cul && ne > cul - nMH &&
            (n < nMH + cul && n > cul - nMH || e < nMH + cul && e > cul - nMH))
        {
            Route[i] = 51; ++x; --y;
        }
        else if (tx > x && ty == y && e < nMH + cul && e > cul - nMH) { Route[i] = 54; ++x; }
        else if (tx > x && ty > y && se < nMH + cul && se > cul - nMH &&
            (s < nMH + cul && s > cul - nMH || e < nMH + cul && e > cul - nMH))
        {
            Route[i] = 57; ++x; ++y;
        }
        else if (tx < x && ty > y && sw < nMH + cul && sw > cul - nMH &&
            (s < nMH + cul && s > cul - nMH || w < nMH + cul && w > cul - nMH))
        {
            Route[i] = 55; --x; ++y;
        }
        else if (tx < x && ty == y && w < nMH + cul && w > cul - nMH) { Route[i] = 52; --x; }
        else if (tx < x && ty < y && nw < nMH + cul && nw > cul - nMH &&
            (n < nMH + cul && n > cul - nMH || w < nMH + cul && w > cul - nMH))
        {
            Route[i] = 49; --x; --y;
        }
        else if (tx > x && ty < y && e < nMH + cul && e > cul - nMH) { Route[i] = 54; ++x; }
        else if (tx > x && ty < y && n < nMH + cul && n > cul - nMH) { Route[i] = 50; --y; }
        else if (tx > x && ty > y && e < nMH + cul && e > cul - nMH) { Route[i] = 54; ++x; }
        else if (tx > x && ty > y && s < nMH + cul && s > cul - nMH) { Route[i] = 56; ++y; }
        else if (tx < x && ty > y && w < nMH + cul && w > cul - nMH) { Route[i] = 52; --x; }
        else if (tx < x && ty > y && s < nMH + cul && s > cul - nMH) { Route[i] = 56; ++y; }
        else if (tx < x && ty < y && w < nMH + cul && w > cul - nMH) { Route[i] = 52; --x; }
        else if (tx < x && ty < y && n < nMH + cul && n > cul - nMH) { Route[i] = 50; --y; }
        else
        {
            if (tx == x + 1 || ty == y + 1 || tx == x - 1 || ty == y - 1)
            {
                Route[i] = 0;
                break;
            }

            if (tx == x && ty > y && se < nMH + cul && se > cul - nMH &&
                (s < nMH + cul && s > cul - nMH || e < nMH + cul && e > cul - nMH))
            {
                Route[i] = 57; ++x; ++y;
            }
            else if (tx == x && ty > y && sw < nMH + cul && sw > cul - nMH &&
                (s < nMH + cul && s > cul - nMH || w < nMH + cul && w > cul - nMH))
            {
                Route[i] = 55; --x; ++y;
            }
            else if (tx == x && ty < y && ne < nMH + cul && ne > cul - nMH &&
                (n < nMH + cul && n > cul - nMH || e < nMH + cul && e > cul - nMH))
            {
                Route[i] = 51; ++x; --y;
            }
            else if (tx == x && ty < y && nw < nMH + cul && nw > cul - nMH &&
                (n < nMH + cul && n > cul - nMH || w < nMH + cul && w > cul - nMH))
            {
                Route[i] = 49; --x; --y;
            }
            else if (tx < x && ty == y && sw < nMH + cul && sw > cul - nMH &&
                (s < nMH + cul && s > cul - nMH || w < nMH + cul && w > cul - nMH))
            {
                Route[i] = 55; --x; ++y;
            }
            else if (tx < x && ty == y && nw < nMH + cul && nw > cul - nMH &&
                (n < nMH + cul && n > cul - nMH || w < nMH + cul && w > cul - nMH))
            {
                Route[i] = 49; --x; --y;
            }
            else if (tx > x && ty == y && se < nMH + cul && se > cul - nMH &&
                (s < nMH + cul && s > cul - nMH || e < nMH + cul && e > cul - nMH))
            {
                Route[i] = 57; ++x; ++y;
            }
            else
            {
                if (tx <= x || ty != y ||
                    ne >= nMH + cul || ne <= cul - nMH ||
                    ((n >= nMH + cul || n <= cul - nMH) && (e >= nMH + cul || e <= cul - nMH)))
                {
                    Route[i] = 0;
                    break;
                }
                Route[i] = 51; ++x; --y;
            }
        }
    }

    if (lastx == x && lasty == y)
        return 0;

    *targetx = x;
    *targety = y;
    return lastx != x || lasty != y;
}

int BASE_GetDistance(int x1, int y1, int x2, int y2)
{
    const int dx = (x1 <= x2) ? x2 - x1 : x1 - x2;
    const int dy = (y1 <= y2) ? y2 - y1 : y1 - y2;

    if (dx <= 6 && dy <= 6)
        return g_pDistanceTable[dy][dx];

    return (dx <= dy) ? dy + 1 : dx + 1;
}

int BASE_GetSpeed(STRUCT_SCORE* score)
{
    int run = score->AttackRun & 0xF;
    if (run < 1) run = 1;
    if (run > 7) run = 7;
    return run;
}

// ---------------------------------------------------------------------------
// Item icon / name / UI string loaders
// ---------------------------------------------------------------------------

int ReadItemicon()
{
    FILE* fp = nullptr;
    fopen_s(&fp, ItemIcon_Path, "rb");
    if (fp)
    {
        fread(g_itemicon, sizeof(g_itemicon), 1u, fp);
        fclose(fp);
    }
    return 1;
}

void ReadItemName()
{
    FILE* fp = nullptr;
    fopen_s(&fp, ItemName_Path, "rb");
    if (!fp)
        return;

    for (int i = 0; i < MAX_ITEMLIST; ++i)
    {
        int  Index = -1;
        char Name[256]{};

        if (!fread(&Index, 4, 1, fp) || !fread(Name, 64, 1, fp))
            break;

        for (int n = 0; n < 62; ++n)
            Name[n] -= n;

        if (Index != -1 && Index < 6500)
        {
            if (strlen(Name) >= 63)
            {
                Name[63] = '\0';
                Name[62] = '\0';
            }
            strcpy(g_pItemList[Index].Name, Name);
        }
    }

    fclose(fp);
}

void ReadUIString()
{
    FILE* pFile = nullptr;
    fopen_s(&pFile, UIString_Path, "rt");

    if (!pFile)
        return;

    char str[128]{};
    while (fgets(str, sizeof(str), pFile))
    {
        char part[64]{};
        int  Index = 0;

        if (sscanf_s(str, "%d %s", &Index, part, 64) != 2)
            continue;

        strncpy_s(g_UIString[Index], part, sizeof(part));
    }

    fclose(pFile);
}

// ---------------------------------------------------------------------------
// Stub / no-op stubs
// ---------------------------------------------------------------------------

char ReadNameFiltraDataBase() { return 1; }
char ReadChatFiltraDataBase() { return 1; }
void EnableSysKey() {}
void DisableSysKey() {}
bool CheckOS() { return false; }

// ---------------------------------------------------------------------------
// Skill helpers
// ---------------------------------------------------------------------------

int IsSkill(int nSkillIndex)
{
    if (nSkillIndex >= 5000 && nSkillIndex <= 5104) return 1;
    if (nSkillIndex >= 5400 && nSkillIndex <= 5447) return 1;
    return 0;
}

int GetSkillIndex(int nSkillIndex)
{
    if (nSkillIndex >= 5400)       nSkillIndex -= 5200;
    else if (nSkillIndex >= 5000)  nSkillIndex -= 5000;
    return nSkillIndex;
}

int IsValidSkill(int nSkillIndex)
{
    if (nSkillIndex >= 0 && nSkillIndex < 104)
    {
        if (nSkillIndex >= 96)
        {
            if (!((1 << (nSkillIndex - 72)) & g_pObjectManager->m_stMobData.LearnedSkill[0]))
                return 0;
        }
        else if (!((1 << (nSkillIndex % 24)) & g_pObjectManager->m_stMobData.LearnedSkill[0]))
        {
            return 0;
        }
        return 1;
    }

    if (nSkillIndex == 205 && g_pObjectManager->m_stMobData.LearnedSkill[1] & 0x20)  return 1;
    if (nSkillIndex == 233 && g_pObjectManager->m_stMobData.LearnedSkill[1] & 0x200) return 1;
    if (nSkillIndex == 238 && g_pObjectManager->m_stMobData.LearnedSkill[1] & 4)     return 1;

    if (nSkillIndex < 200 || nSkillIndex >= 247)                              return 0;
    if (nSkillIndex == 205 || nSkillIndex == 233 || nSkillIndex == 238)       return 0;

    return ((1 << (4 * ((nSkillIndex - 200) / 4))) & g_pObjectManager->m_stMobData.LearnedSkill[1]) != 0;
}

int IsValidClassSkill(int nSkillIndex)
{
    if (nSkillIndex >= 0 && nSkillIndex < 96)
    {
        if (nSkillIndex / 24 != (unsigned __int8)g_pObjectManager->m_stMobData.Class)
            return 0;
        if (!((1 << (nSkillIndex % 24)) & g_pObjectManager->m_stMobData.LearnedSkill[0]))
            return 0;

        const int kind = nSkillIndex % 24 / 8 + 1;
        if (kind <= 0 || kind > 3)
            return 0;
    }
    else if (nSkillIndex < 96 || nSkillIndex > 103 &&
        ((1 << (nSkillIndex - 72)) & g_pObjectManager->m_stMobData.LearnedSkill[0]))
    {
        return 1;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// String validation helpers
// ---------------------------------------------------------------------------

int IsClearString(char* str, int target)
{
    const int len = (int)strlen(str);

    for (int pos = 0; pos < len; ++pos)
    {
        if (str[pos] >= 0)
        {
            if (pos >= target)
                return 1;
        }
        else
        {
            if (pos == target)     return 0;
            if (pos == target + 1) return 1;
            ++pos;
        }
    }

    return 1;
}

int IsClearString2(char* str, int nTarget)
{
    if (!str)
        return 1;

    char* pNextRightChar = CharNext(&str[nTarget]);
    const int nLen = (int)(pNextRightChar - &str[nTarget]);
    const int nLen2 = (int)(pNextRightChar - CharPrev(str, pNextRightChar));

    if (nLen == 1 && nLen2 == 2) return 0;
    if (nLen != 2 || nLen2 != 1) return 1;

    return 0;
}

// ---------------------------------------------------------------------------
// Server list initialisation
// ---------------------------------------------------------------------------

int BASE_InitializeServerList()
{
    memset(&g_pServerList, 0, sizeof(g_pServerList));

    snprintf(g_pServerList[0][0], 64, "https://wydglobal.com.br/"); // Link da barra
    snprintf(g_pServerList[0][1], 64, "172.29.16.1");               // Ip do servidor

    return 1;
}

// ---------------------------------------------------------------------------
// Zone / guild helpers
// BUG FIX: comparacao de bounds corrigida — original tratava vx2/vy1/vy2
//          como booleans em vez de comparar com x e y
// ---------------------------------------------------------------------------

int BASE_GetVillage(int x, int y)
{
    for (int i = 0; i < 5; ++i)
    {
        if (x >= g_pGuildZone[i].vx1 && x <= g_pGuildZone[i].vx2 &&
            y >= g_pGuildZone[i].vy1 && y <= g_pGuildZone[i].vy2)
        {
            return i;
        }
    }
    return MAX_GUILDZONE;
}

int BASE_GetSubGuild(int item)
{
    int ret = 0;
    if (item >= 3 && item <= 8)
        ret = item % 3 + 1;
    return ret;
}

// ---------------------------------------------------------------------------
// Item colour helpers
// ---------------------------------------------------------------------------

unsigned int BASE_GetItemTenColor(STRUCT_ITEM* pItem)
{
    if (BASE_GetItemSanc(pItem) <= 9)
        return 0;

    unsigned char sanc = 0;

    for (int i = 0; i < 3; ++i)
    {
        const auto eff = pItem->stEffect[i].cEffect;
        if (eff == EF_SANC || (eff >= 115 && eff <= 126))
        {
            sanc = pItem->stEffect[i].cValue;
            break;
        }
    }

    return (sanc - 230) % 4 + 5;
}

int BASE_GetItemColorEffect(STRUCT_ITEM* item)
{
    int effect = 0;

    const bool noDirectSanc = (item->stEffect[0].cEffect != EF_SANC &&
        item->stEffect[1].cEffect != EF_SANC &&
        item->stEffect[2].cEffect != EF_SANC);

    if (noDirectSanc)
    {
        for (int i = 0; i < 3; ++i)
        {
            const auto eff = item->stEffect[i].cEffect;
            if (eff >= 115 && eff <= 126)
            {
                effect = eff;
                break;
            }
        }
    }
    else
    {
        if (item->stEffect[0].cEffect == EF_SANC)
            effect = item->stEffect[0].cEffect;
        else if (item->stEffect[1].cEffect == EF_SANC)
            effect = item->stEffect[1].cEffect;
        else
            effect = item->stEffect[2].cEffect;
    }

    return effect;
}

// ---------------------------------------------------------------------------
// Name / chat validation
// ---------------------------------------------------------------------------

char BASE_CheckValidString(char* name)
{
    const int size = (int)strlen(name);
    if (size < 4 || size >= 16)
        return 0;

    for (int j = 0; j < size; ++j)
    {
        const char x = name[j];
        if (x < 0)
        {
            if (!name[++j])
                return 0;
        }
        else if ((x < 'a' || x > 'z') &&
            (x < 'A' || x > 'Z') &&
            (x < '0' || x > '9') &&
            x != '-')
        {
            return 0;
        }
    }

    return 1;
}

char* BASE_TransCurse(char* sz)
{
    if (!sz)
        return nullptr;

    bool bFind;
    do
    {
        bFind = false;

        for (size_t i = 0; i < g_pCurseList.dnum; ++i)
        {
            if (strlen(g_pCurseList.pCurseList[i].szOriginal) == 0)
                return sz;

            char* szDest = strstr(sz, g_pCurseList.pCurseList[i].szOriginal);
            if (!szDest)
                continue;

            const int nOriLen = (int)strlen(g_pCurseList.pCurseList[i].szOriginal);
            if (!IsClearString2(sz, (int)(szDest - sz)))
                continue;

            char szNext[128]{};
            memcpy(szNext, sz, szDest - sz);
            strcat(szNext, g_pCurseList.pCurseList[i].szTrans);
            strcat(szNext, &szDest[nOriLen]);
            sprintf(sz, szNext);
            bFind = true;
            break;
        }
    } while (bFind);

    return sz;
}

// ---------------------------------------------------------------------------
// Attribute accessors
// BUG FIX: segundo bound check usava x em vez de y (y >= 0 && x <= 4096)
// ---------------------------------------------------------------------------

char BASE_GetAttribute(int x, int y)
{
    if (x >= 0 && x <= 4096 && y >= 0 && y <= 4096)
        return g_pAttribute[y / 4 & 1023][x / 4 & 1023];

    return 0;
}

char BASE_GetAttr(int nX, int nY)
{
    return g_pAttribute[nY / 4 % 1024][nX / 4 % 1024];
}

int BASE_ReadTOTOList(char* szFileName)
{
    return 0;
}

// ---------------------------------------------------------------------------
// Static item ability (no bonus effects)
// ---------------------------------------------------------------------------

int BASE_GetStaticItemAbility(STRUCT_ITEM* item, char Type)
{
    int value = 0;
    const int idx = item->sIndex;

    if (idx <= 0 || idx > MAX_ITEMLIST)
        return value;

    if (idx >= 3200 && idx <= 3300)
        return 0;

    const int nPos = g_pItemList[idx].nPos;

    if (Type == EF_LEVEL)    value += g_pItemList[idx].nReqLvl;
    if (Type == EF_REQ_STR)  value += g_pItemList[idx].nReqStr;
    if (Type == EF_REQ_INT)  value += g_pItemList[idx].nReqInt;
    if (Type == EF_REQ_DEX)  value += g_pItemList[idx].nReqDex;
    if (Type == EF_REQ_CON)  value += g_pItemList[idx].nReqCon;
    if (Type == EF_POS)      value += g_pItemList[idx].nPos;

    if (Type != EF_INCUBATE)
    {
        for (int i = 0; i < 12; ++i)
        {
            if (g_pItemList[idx].stEffect[i].sEffect != Type)
                continue;

            int tvalue = g_pItemList[idx].stEffect[i].sValue;
            if (Type == EF_ATTSPEED && tvalue == 1)
                tvalue = 10;
            value += tvalue;
        }
    }

    if (idx >= 2330 && idx < 2390)
    {
        if (Type == EF_MOUNTHP)   return item->stEffect[0].sValue;
        if (Type == EF_MOUNTSANC) return item->stEffect[1].cEffect;
        if (Type == EF_MOUNTLIFE) return item->stEffect[1].cValue;
        if (Type == EF_MOUNTFEED) return item->stEffect[2].cEffect;
        if (Type == EF_MOUNTKILL) return item->stEffect[2].cValue;

        if (idx < 2362 || idx >= 2390 || item->stEffect[0].sValue <= 0)
            return value;

        const int lv = item->stEffect[1].cEffect;
        const int cd = idx - 2360;

        if (Type == EF_DAMAGE)    return g_pMountBonus[cd][0] * (lv + 20) / 100;
        if (Type == EF_MAGIC)     return g_pMountBonus[cd][1] * (lv + 15) / 100;
        if (Type == EF_PARRY)     return g_pMountBonus[cd][2];
        if (Type == EF_RESISTALL) return g_pMountBonus[cd][3];

        return value;
    }

    if (idx >= 3980 && idx <= 3994)
    {
        const int cd = idx - 3980;
        if (Type == EF_DAMAGE)    return g_pMountBonus2[cd][0];
        if (Type == EF_MAGIC)     return g_pMountBonus2[cd][1];
        if (Type == EF_PARRY)     return g_pMountBonus2[cd][2];
        if (Type == EF_RESISTALL) return g_pMountBonus2[cd][3];
        return value;
    }

    int sanc = BASE_GetItemSanc(item);

    if (sanc >= 9 && (nPos & 0xF00))
        sanc++;

    const bool skipSanc =
        (Type == EF_GRID || Type == EF_CLASS || Type == EF_POS ||
            Type == EF_WTYPE || Type == EF_RANGE || Type == EF_LEVEL ||
            Type == EF_REQ_STR || Type == EF_REQ_INT || Type == EF_REQ_DEX ||
            Type == EF_REQ_CON || Type == EF_VOLATILE || Type == EF_INCUBATE ||
            Type == EF_INCUDELAY || Type == EF_PREVBONUS ||
            Type == EF_REGENMP || Type == EF_REGENHP);

    if (sanc && !skipSanc)
    {
        if (sanc > 10)
        {
            int UpSanc = sanc - 10;
            switch (UpSanc)
            {
            case 1: UpSanc = 220; break;
            case 2: UpSanc = 250; break;
            case 3: UpSanc = 280; break;
            case 4: UpSanc = 320; break;
            case 5: UpSanc = 370; break;
            case 6: UpSanc = 400; break;
            }
            value = UpSanc * 10 * value / 100 / 10;
        }
        else
        {
            value = value * (sanc + 10) / 10;
        }
    }

    if (Type == EF_RUNSPEED)
    {
        if (value >= 3)  value = 2;
        if (value > 0 && sanc >= 9) value++;
    }

    if ((Type == EF_REGENMP || Type == EF_REGENHP) && sanc > 0)
        value *= sanc;

    return value;
}

// ---------------------------------------------------------------------------
// Zone boundary check
// BUG FIX: LOG_WRITELOG agora passa nX e nY como argumentos
// ---------------------------------------------------------------------------

int BASE_IsInLowZone(int nX, int nY)
{
    const int nX4 = nX / 4;
    const int nY4 = nY / 4;

    if (nY4 < 1024 && nX4 < 1024 && nY4 >= 0 && nX4 >= 0)
        return g_pAttribute[nY4][nX4] < 0;

    LOG_WRITELOG("\nWrong Position [X:%d Y:%d]\n", nX, nY);
    MessageBox(g_pApp->m_hWnd, "Wrong Character Information.", "Error", MB_SYSTEMMODAL);
    PostMessage(g_pApp->m_hWnd, 16, 0, 0);
    return 0;
}

// ---------------------------------------------------------------------------
// Carry / trade / cargo helpers
// ---------------------------------------------------------------------------

int BASE_GetItemAmount(STRUCT_ITEM* item)
{
    return BASE_GetEffectValue(item, EF_AMOUNT);
}

int BASE_CanCarry(STRUCT_ITEM* Carry, int pos)
{
    if (pos < 30)                                      return 1;
    if (pos / 15 == 2 && Carry[60].sIndex != 3467)    return 0;
    if (pos / 15 != 3 || Carry[61].sIndex == 3467)    return 1;
    return 0;
}

int BASE_CanTrade(STRUCT_ITEM* Dest, STRUCT_ITEM* Carry, char* MyTrade, STRUCT_ITEM* OpponentTrade)
{
    STRUCT_ITEM OpponentTemp[MAX_TRADE]{};
    memcpy(Dest, Carry, sizeof(STRUCT_ITEM) * MAX_CARRY);

    for (int i = 0; i < MAX_TRADE; ++i)
    {
        const int pos = (unsigned char)MyTrade[i];
        if (pos != (unsigned char)-1)
            BASE_ClearItem(&Dest[pos]);
    }

    BASE_SortTradeItem(OpponentTemp, EF_GRID);

    for (int i = 0; i < MAX_TRADE; ++i)
    {
        if (!OpponentTemp[i].sIndex)
            continue;

        int j = 0;
        for (j = 0; j < MAX_VISIBLE_CARRY; ++j)
        {
            if (!Dest[j].sIndex && BASE_CanCarry(Dest, j))
            {
                Dest[j] = OpponentTemp[i];
                break;
            }
        }

        if (j == MAX_VISIBLE_CARRY)
            return 0;
    }

    return 1;
}

// BUG FIX: sizeof(item) era sizeof do ponteiro (4 ou 8 bytes).
//          Corrigido para sizeof(STRUCT_ITEM).
void BASE_ClearItem(STRUCT_ITEM* item)
{
    memset(item, 0, sizeof(STRUCT_ITEM));
}

void BASE_SortTradeItem(STRUCT_ITEM* Item, int Type)
{
    int Buffer[15]{};

    for (int i = 0; i < MAX_TRADE; ++i)
    {
        Buffer[i] = Item[i].sIndex
            ? BASE_GetItemAbility(&Item[i], Type)
            : -1;
    }

    STRUCT_ITEM ItemTemp[MAX_TRADE]{};

    for (int i = 0; i < MAX_TRADE; ++i)
    {
        int maxIdx = 0;
        int maxVal = -1;

        for (int j = 0; j < MAX_TRADE; ++j)
        {
            if (Buffer[j] > maxVal)
            {
                maxIdx = j;
                maxVal = Buffer[j];
            }
        }

        if (maxVal == -1)
            break;

        Buffer[maxIdx] = -1;
        ItemTemp[i] = Item[maxIdx];
    }

    memcpy(Item, ItemTemp, sizeof(ItemTemp));
}

int BASE_CanCargo(STRUCT_ITEM* item, STRUCT_ITEM* cargo, int DestX, int DestY)
{
    BASE_GetItemAbility(item, EF_GRID);

    char SourGrid[4][2]{};
    char CargoGrid[14][9]{};

    for (int i = 0; i < MAX_CARGO; ++i)
    {
        if (!cargo[i].sIndex)
            continue;

        char TempGrid[4][2]{};
        memcpy((char*)TempGrid,
            (char*)g_pItemGrid[BASE_GetItemAbility(&cargo[i], EF_GRID)],
            8u);

        const int tx = i % 9;
        const int ty = i / 9;

        for (int yy = 0; yy < 4; ++yy)
        {
            for (int xx = 0; xx < 2; ++xx)
            {
                if (TempGrid[yy][xx] == 1 &&
                    ty + yy >= 0 && tx + xx >= 0 &&
                    ty + yy < 14 && tx + xx < 9)
                {
                    CargoGrid[ty][9 * yy + tx + xx] = 1;
                }
            }
        }
    }

    for (int j = 0; j < 4; ++j)
    {
        for (int k = 0; k < 2; ++k)
        {
            if (SourGrid[j][k] != 1)
                continue;

            if (DestY + j < 0 || DestX + k < 0 || DestY + j >= 14 || DestX + k >= 9)
                return 0;

            if (CargoGrid[DestY][9 * j + DestX + k] == 1)
                return 0;
        }
    }

    return 1;
}

// ---------------------------------------------------------------------------
// Equipment eligibility
// ---------------------------------------------------------------------------

int BASE_CanEquip(STRUCT_ITEM* item, STRUCT_SCORE* score, int Pos, int Class,
    STRUCT_ITEM* pBaseEquip, int OriginalFace, bool hasSoulLimitSkill)
{
    const int idx = item->sIndex;
    if (idx <= 0 || idx >= 6500) return FALSE;

    const int nUnique = g_pItemList[idx].nUnique;
    if (Pos == 15) return FALSE;

    if (Pos != -1)
    {
        const int tpos = BASE_GetItemAbility(item, EF_POS);
        if (!((tpos >> Pos) & 1)) return FALSE;

        if (Pos == 6 || Pos == 7)
        {
            const int OtherPos = (Pos == 6) ? 7 : 6;
            const int OtherIdx = pBaseEquip[OtherPos].sIndex;

            if (OtherIdx > 0 && OtherIdx < MAX_ITEMLIST)
            {
                const int nUnique2 = g_pItemList[OtherIdx].nUnique;
                const int otherpos = BASE_GetItemAbility(&pBaseEquip[OtherPos], EF_POS);

                if (tpos == 64 || otherpos == 64)
                {
                    if (nUnique == 46)
                    {
                        if (otherpos != 128) return FALSE;
                    }
                    else if (nUnique2 == 46)
                    {
                        if (tpos != 128) return FALSE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
            }
        }
    }

    if ((Class >= 22 && Class <= 25) || Class == 32)
        Class = OriginalFace;

    const int trans = Class % 10;

    if (Pos == 1 && trans > 5
        && item->sIndex != 747
        && item->sIndex != 3500
        && item->sIndex != 3507
        && item->sIndex != 3501
        && item->sIndex != 3502
        && item->sIndex != 3303)
    {
        return FALSE;
    }

    const int transitem = BASE_GetItemAbility(item, EF_TRANS);
    switch (transitem)
    {
    case 1:
        if (trans < 6)                         return FALSE;
        break;
    case 2:
        if (trans >= 6)                        return FALSE;
        break;
    case 3:
        if (trans < 6)                         return FALSE;
        if (!hasSoulLimitSkill)                return FALSE;
        break;
    }

    if (!((BASE_GetItemAbility(item, EF_CLASS) >> (Class / 10)) & 1))
    {
        if (trans <= 5) return FALSE;

        const int pos = BASE_GetItemAbility(item, EF_POS);
        if (pos != 64 && pos != 128 && pos != 192)
            return FALSE;
    }

    if (nUnique % 10 >= 9 && nUnique < 40 && trans < 6)
        return FALSE;

    int lvl = BASE_GetItemAbility(item, EF_LEVEL);
    int str = BASE_GetItemAbility(item, EF_REQ_STR);
    int spt = BASE_GetItemAbility(item, EF_REQ_INT);
    int agi = BASE_GetItemAbility(item, EF_REQ_DEX);
    int con = BASE_GetItemAbility(item, EF_REQ_CON);

    const int wtype = BASE_GetItemAbility(item, EF_WTYPE);
    const int weapontype = wtype % 10;
    const int modweapon = wtype % 10;
    const int divweapon = wtype % 10 / 10;

    if (Pos == 7 && weapontype)
    {
        int rate = 100;
        if (divweapon || modweapon <= 1)
        {
            if (divweapon == 6 && modweapon > 1)
                rate = 150;
        }
        else
        {
            rate = 130;
        }

        lvl = rate * lvl / 100;
        str = rate * str / 100;
        spt = rate * spt / 100;
        agi = rate * agi / 100;
        con = rate * con / 100;
    }

    if (trans < 5)
    {
        if (lvl > score->Level) return FALSE;
        if (str > score->Str)   return FALSE;
        if (spt > score->Int)   return FALSE;
        if (agi > score->Dex)   return FALSE;
        if (con > score->Con)   return FALSE;
    }

    return TRUE;
}

// ---------------------------------------------------------------------------
// Item colour computation
// ---------------------------------------------------------------------------

unsigned int BASE_GetItemColor(STRUCT_ITEM* item)
{
    if (!item)
        return 0xFF99EE99;

    unsigned int dwColor = 0xFFAAAAFF;
    int          nMaxValue = 0;
    unsigned int dwMaxParm = 0;

    for (int i = 0; i < 49; ++i)
    {
        auto parm = dwEFParam[i];
        if (parm == 45) parm = 69;
        if (parm == 46) parm = 70;

        const int nPos = BASE_GetItemAbility(item, 17);

        int          nValue2 = 0;
        int          nValue3 = 0;
        unsigned int dwTempColor = 0;

        if (parm == 42 || parm == 53 || (nPos == 32 && parm == 2))
        {
            nValue2 = BASE_GetItemAbility(item, parm);
            nValue3 = BASE_GetItemAbilityNosanc(item, parm);
            dwTempColor = BASE_GetOptionColor(nPos, parm, nValue3);
        }
        else
        {
            nValue2 = BASE_GetBonusItemAbility(item, parm);
            nValue3 = BASE_GetBonusItemAbilityNosanc(item, parm);
            dwTempColor = BASE_GetOptionColor(nPos, parm, nValue3);
        }

        if (dwMaxParm && (unsigned __int8)dwMaxParm == parm)
        {
            nMaxValue += nValue3;
            dwColor = BASE_GetOptionColor(nPos, parm, nValue2);
        }
        else if (nValue3)
        {
            if (BASE_GetColorCount(dwColor) < BASE_GetColorCount(dwTempColor))
            {
                nMaxValue = nValue3;
                dwMaxParm = parm;
                dwColor = dwTempColor;
            }
        }
    }

    return dwColor;
}

int BASE_GetColorCount(unsigned int dwColor)
{
    if (dwColor == 0xFFAAAAFF) return 1;
    if (dwColor == 0xFF99EE99) return 2;
    if (dwColor == 0xFFFFFFAA) return 3;
    if (dwColor == 0xFFFFAA00) return 4;
    return 0;
}

// ---------------------------------------------------------------------------
// Skill damage helpers
// ---------------------------------------------------------------------------

int BASE_GetManaSpent(int SkillNumber, int SaveMana, int Special)
{
    return g_pSpell[SkillNumber].ManaSpent * (Special / 2 + 100) / 100 * (100 - SaveMana) / 100;
}

int BASE_GetSkillDamage(int dam, int ac, int combat)
{
    if (combat > 15) combat = 15;

    int tdam = (rand() % (21 - combat) + combat + 90) * (dam - ac / 2) / 100;

    if (tdam < -50)                    tdam = 0;
    else if (tdam >= -50 && tdam < 0)  tdam = (tdam + 50) / 10;
    else if (tdam >= 0 && tdam <= 45)  tdam = 5 * tdam / 4 + 5;

    if (tdam <= 0) tdam = 1;

    return tdam;
}

int BASE_GetSkillDamage(int skillnum, STRUCT_MOB* mob, int weather, int weapondamage, int OriginalFace)
{
    const int instanceindex = g_pSpell[skillnum].InstanceType;

    int level = mob->CurrentScore.Level;
    if (level < 0)    level = 0;
    if (level >= 400) level = 400;

    const int special = mob->CurrentScore.Special[skillnum % 24 / 8 + 1];
    const int base = g_pSpell[skillnum].InstanceValue;
    const int affectbase = g_pSpell[skillnum].AffectValue;
    const int skillclass = skillnum / 8 % 3;
    int       dam = 0;

    if (instanceindex == 0)
    {
        switch (skillnum)
        {
        case 11: dam = affectbase + special / 10;        break;
        case 13: dam = affectbase + 3 * special;         break;
        case 41: dam = special / 25 + 2;                 break;
        case 43: dam = affectbase + special / 3 + 15;    break;
        case 44: dam = 5 * (special / 3 + 15);           break;
        case 45: dam = affectbase + special / 10;        break;
        }
    }
    else if (instanceindex >= 1 && instanceindex <= 5)
    {
        const int skind = skillnum / 8;
        const int trans = (OriginalFace % 10 > 5) ? 1 : 0;

        if (!trans)
        {
            if (skillnum == 97)
                dam = base + 15 * level;
            else if (!mob->Class && skind == 1)
                dam = 3 * weapondamage + 3 * mob->CurrentScore.Str + level / 2 + special + base;
            else if (!mob->Class && skind != 1)
                dam = weapondamage + mob->CurrentScore.Int / 4 + level / 2 + special + base + mob->CurrentScore.Int / 40;
            else
            {
                switch (mob->Class)
                {
                case 1: dam = mob->CurrentScore.Int / 30 + mob->CurrentScore.Int / 3 + level / 2 + special + base; break;
                case 2: dam = mob->CurrentScore.Int / 30 + mob->CurrentScore.Int / 3 + level / 2 + special + base; break;
                case 3: dam = (skillnum == 79) ? mob->CurrentScore.Damage
                    : 3 * weapondamage + 3 * mob->CurrentScore.Str + level / 2 + special + base;
                    break;
                }
            }
        }
        else if (skillnum == 97)
        {
            dam = base + 15 * level;
        }
        else if (!mob->Class && skind == 1)
        {
            dam = 3 * weapondamage + 3 * mob->CurrentScore.Str + level + special + base;
        }
        else if (!mob->Class && skind != 1)
        {
            dam = weapondamage + mob->CurrentScore.Int / 4 + level + special + base + mob->CurrentScore.Int / 40;
        }
        else
        {
            switch (mob->Class)
            {
            case 1: dam = mob->CurrentScore.Int / 30 + mob->CurrentScore.Int / 3 + level + base + 2 * special; break;
            case 2: dam = mob->CurrentScore.Int / 30 + mob->CurrentScore.Int / 3 + level + base + 2 * special; break;
            case 3: dam = (skillnum == 79) ? mob->CurrentScore.Damage
                : 3 * weapondamage + 3 * mob->CurrentScore.Str + level / 2 + special + base;
                break;
            }
        }

        if (weather == 1)
        {
            if (instanceindex == 2) dam = 90 * dam / 100;
            if (instanceindex == 5) dam = 130 * dam / 100;
        }
        else if (weather == 2 && instanceindex == 3)
        {
            dam = 120 * dam / 100;
        }

        if (skillnum != 97)
        {
            if (skillnum == 79)
                return dam;

            if (!mob->Class && skind == 1 || mob->Class == 3)
                dam = 5 * dam / 4;
            else
                dam = 5 * (dam * (4 * (unsigned __int8)mob->Magic + 100) / 100) / 4;
        }

        if ((1 << (8 * skillclass + 7)) & mob->LearnedSkill[0])
        {
            switch (mob->Class)
            {
            case 0:
                if (!skillclass)          dam = 115 * dam / 100;
                else if (skillclass == 1) dam = 120 * dam / 100;
                else if (skillclass == 2) dam = 115 * dam / 100;
                break;
            case 1:
                if (!skillclass)          dam = 110 * dam / 100;
                else if (skillclass == 1) dam = 115 * dam / 100;
                else if (skillclass == 2) dam = 115 * dam / 100;
                break;
            case 2:
                if (!skillclass)          dam = 110 * dam / 100;
                break;
            case 3:
                if (!skillclass)          dam = 110 * dam / 100;
                else if (skillclass == 1) dam = 110 * dam / 100;
                else if (skillclass == 2) dam = 120 * dam / 100;
                break;
            }
        }
    }
    else if (instanceindex == 6)
    {
        dam = base + 3 * special / 2;
        if (skillnum == 29 && mob->LearnedSkill[0] & 0x80)
            dam = 120 * dam / 100;
    }
    else if (instanceindex == 11)
    {
        dam = g_pSpell[skillnum].InstanceValue;
    }
    else
    {
        dam = 2 * (unsigned char)mob->Magic;
    }

    return dam;
}

// ---------------------------------------------------------------------------
// Equipment eligibility (result struct variant)
// ---------------------------------------------------------------------------

int BASE_CanEquip_RecvRes(STRUCT_REQ* req, STRUCT_ITEM* item, STRUCT_SCORE* score,
    int Pos, int Class, STRUCT_ITEM* pBaseEquip, int OriginalFace)
{
    if (!req) return 0;

    req->Class = req->Level = req->Str = req->Int = req->Dex = req->Con = 0;

    const int idx = item->sIndex;
    if (idx <= 0 || idx >= 6500) return 0;

    const int nUnique = g_pItemList[idx].nUnique;
    if (Pos == 15) return 0;

    if (Pos != -1)
    {
        const int nPos = BASE_GetItemAbility(item, 17);
        if (!((nPos >> Pos) & 1)) return 0;

        if (Pos == 6 || Pos == 7)
        {
            const int OtherPos = (Pos == 6) ? 7 : 6;
            const int OtherIdx = pBaseEquip[OtherPos].sIndex;

            if (OtherIdx > 0 && OtherIdx < 6500)
            {
                const int nUnique2 = g_pItemList[OtherIdx].nUnique;
                const int otherpos = BASE_GetItemAbility(&pBaseEquip[OtherPos], 17);

                if (nPos == 64 || otherpos == 64)
                {
                    if (nUnique == 46)
                    {
                        if (otherpos != 128) return 0;
                    }
                    else
                    {
                        if (nUnique2 != 46) return 0;
                        if (nPos != 128)    return 0;
                    }
                }
            }
        }
    }

    if ((Class >= 22 && Class <= 25) || Class == 32)
        Class = OriginalFace;

    int mount = 0;
    if (idx >= 2300 && idx < 2390)
        mount = (idx - 2300) % 30;

    const int trans = Class % 10;

    if (Pos == 1 && item->sIndex != 747 && trans > 5)   return 0;
    if ((mount == 19 || mount == 20) && trans <= 5)      return 0;

    req->Class = 1;

    const int transitem = BASE_GetItemAbility(item, 112);
    if (transitem == 1 && trans < 6)   req->Class = 0;
    if (transitem == 2 && trans >= 6)  req->Class = 0;

    const int cls = (BASE_GetItemAbility(item, 18) >> (Class / 10)) & 1;
    const int tpos = BASE_GetItemAbility(item, 17);

    if (!cls)
    {
        if (trans > 5)
        {
            if (tpos != 64 && tpos != 128 && tpos != 192)
                req->Class = 0;
        }
        else
        {
            req->Class = 0;
        }
    }

    if (Class <= 31 && Class % 10 != 1 && tpos == 2)
        req->Class = 0;

    if (nUnique % 10 >= 9 && nUnique < 40 && trans < 6)
        req->Class = 0;

    int lvl = BASE_GetItemAbility(item, 1);
    int str = BASE_GetItemAbility(item, 22);
    int spt = BASE_GetItemAbility(item, 23);
    int agi = BASE_GetItemAbility(item, 24);
    int con = BASE_GetItemAbility(item, 25);

    const int wtype = BASE_GetItemAbility(item, 21);
    const int modweapon = wtype % 10;
    const int divweapon = wtype % 10 / 10;

    if (Pos == 7 && modweapon)
    {
        int rate = 100;
        if (divweapon || modweapon <= 1)
        {
            if (divweapon == 6 && modweapon > 1) rate = 150;
        }
        else
        {
            rate = 130;
        }

        lvl = rate * lvl / 100;
        str = rate * str / 100;
        spt = rate * spt / 100;
        agi = rate * agi / 100;
        con = rate * con / 100;
    }

    if (score->Level >= lvl) req->Level = 1;
    if (score->Str >= str) req->Str = 1;
    if (score->Int >= spt) req->Int = 1;
    if (score->Dex >= agi) req->Dex = 1;
    if (score->Con >= con) req->Con = 1;

    return 1;
}

// ---------------------------------------------------------------------------
// Bonus item ability (no-sanc and with-sanc variants)
// ---------------------------------------------------------------------------

int BASE_GetBonusItemAbilityNosanc(STRUCT_ITEM* item, char Type)
{
    const int idx = item->sIndex;

    if (idx <= 0 || idx > MAX_ITEMLIST)              return 0;
    if (idx >= 3200 && idx <= 3300)                  return 0;
    if (idx >= 2330 && idx < 2390)                   return 0;
    if (idx >= 3980 && idx < 4000)                   return 0;

    int value = 0;

    for (int i = 0; i < 3; ++i)
    {
        if (item->stEffect[i].cEffect != Type)
            continue;

        int tvalue = item->stEffect[i].cValue;
        if (Type == EF_ATTSPEED && tvalue == 1)
            tvalue = 10;
        value += tvalue;
    }

    return value;
}

int BASE_GetBonusItemAbility(STRUCT_ITEM* item, char Type)
{
    const int idx = item->sIndex;

    if (idx <= 0 || idx > 6500)                      return 0;
    if (idx >= 3200 && idx <= 3300)                  return 0;
    if (idx >= 2330 && idx < 2390)                   return 0;
    if (idx >= 3980 && idx < 4000)                   return 0;

    int value = 0;
    const int nPos = g_pItemList[idx].nPos;

    for (int i = 0; i < 3; ++i)
    {
        if (item->stEffect[i].cEffect == Type)
        {
            int tvalue = item->stEffect[i].cValue;
            if (Type == EF_ATTSPEED && tvalue == 1)
                tvalue = 10;
            value += tvalue;
        }
    }

    int sanc = BASE_GetItemSanc(item);

    if (sanc >= 9 && (nPos & 0xF00))
        sanc++;

    const bool skipSanc =
        (Type == EF_GRID || Type == EF_CLASS || Type == EF_POS ||
            Type == EF_WTYPE || Type == EF_RANGE || Type == EF_LEVEL ||
            Type == EF_REQ_STR || Type == EF_REQ_INT || Type == EF_REQ_DEX ||
            Type == EF_REQ_CON || Type == EF_VOLATILE || Type == EF_INCUBATE ||
            Type == EF_INCUDELAY || Type == EF_PREVBONUS);

    if (sanc && !skipSanc)
    {
        if (sanc > 10)
        {
            int UpSanc = sanc - 10;
            switch (UpSanc)
            {
            case 1: UpSanc = 220; break;
            case 2: UpSanc = 250; break;
            case 3: UpSanc = 280; break;
            case 4: UpSanc = 320; break;
            case 5: UpSanc = 370; break;
            case 6: UpSanc = 400; break;
            }
            value = UpSanc * 10 * value / 100 / 10;
        }
        else
        {
            value = value * (sanc + 10) / 10;
        }
    }

    return value;
}

// BUG FIX: `itemId == EF_ATTSPEED` corrigido para `type == EF_ATTSPEED`
int BASE_GetItemAbilityNosanc(STRUCT_ITEM* item, char type)
{
    const int itemId = item->sIndex;
    int       value = 0;

    if (itemId < 0 || itemId >= MAX_ITEMLIST)
        return value;

    const int nUnique = g_pItemList[itemId].nUnique;
    const int nPos = g_pItemList[itemId].nPos;

    if ((type == EF_DAMAGEADD || type == EF_MAGICADD) && (nUnique < 41 || nUnique > 50))
        return value;

    if (type == EF_CRITICAL &&
        (item->stEffect[1].cEffect == EF_CRITICAL2 || item->stEffect[2].cEffect == EF_CRITICAL2))
    {
        type = EF_CRITICAL2;
    }

    if (type == EF_DAMAGE && nPos == 32 &&
        (item->stEffect[1].cEffect == EF_DAMAGE2 || item->stEffect[2].cEffect == EF_DAMAGE2))
    {
        type = EF_DAMAGE2;
    }

    if (type == EF_MPADD &&
        (item->stEffect[1].cEffect == EF_MPADD2 || item->stEffect[2].cEffect == EF_MPADD2))
    {
        type = EF_MPADD2;
    }

    if (type == EF_HPADD &&
        (item->stEffect[1].cEffect == EF_HPADD2 || item->stEffect[2].cEffect == EF_HPADD2))
    {
        type = EF_HPADD2;
    }

    if (type == EF_ACADD &&
        (item->stEffect[1].cEffect == EF_ACADD2 || item->stEffect[2].cEffect == EF_ACADD2))
    {
        type = EF_ACADD2;
    }

    if (type == EF_LEVEL)    value += g_pItemList[itemId].nReqLvl;
    if (type == EF_REQ_STR)  value += g_pItemList[itemId].nReqStr;
    if (type == EF_REQ_INT)  value += g_pItemList[itemId].nReqInt;
    if (type == EF_REQ_DEX)  value += g_pItemList[itemId].nReqDex;
    if (type == EF_REQ_CON)  value += g_pItemList[itemId].nReqCon;
    if (type == EF_POS)      value += g_pItemList[itemId].nPos;

    if (type != EF_INCUBATE)
    {
        for (int i = 0; i < 12; ++i)
        {
            if (g_pItemList[itemId].stEffect[i].sEffect != type)
                continue;

            if (g_pItemList[itemId].stEffect[i].sEffect == EF_HPADD ||
                g_pItemList[itemId].stEffect[i].sEffect == EF_HPADD2)
            {
                continue;
            }

            int tvalue = g_pItemList[itemId].stEffect[i].sValue;
            if (type == EF_ATTSPEED && tvalue == 1)  // BUG FIX: era itemId ==
                tvalue = 10;
            value += tvalue;
        }
    }

    if (itemId >= 2330 && itemId < 2390)
    {
        if (type == EF_MOUNTHP)   return item->stEffect[0].sValue;
        if (type == EF_MOUNTSANC) return item->stEffect[1].cEffect;
        if (type == EF_MOUNTLIFE) return item->stEffect[1].cValue;
        if (type == EF_MOUNTFEED) return item->stEffect[2].cEffect;
        if (type == EF_MOUNTKILL) return item->stEffect[2].cValue;

        if (itemId < 2362 || itemId >= 2390 || item->stEffect[0].sValue <= 0)
            return value;

        const int lv = item->stEffect[1].cEffect;
        const int cd = itemId - 2360;

        if (type == EF_DAMAGE)    return g_pMountBonus[cd][0] * (lv + 20) / 100;
        if (type == EF_MAGIC)     return g_pMountBonus[cd][1] * (lv + 15) / 100;
        if (type == EF_PARRY)     return g_pMountBonus[cd][2];
        if (type == EF_RESISTALL) return g_pMountBonus[cd][3];

        return value;
    }

    if (itemId >= 3980 && itemId <= 3994)
    {
        const int cd = itemId - 3980;
        if (type == EF_DAMAGE)    return g_pMountBonus2[cd][0];
        if (type == EF_MAGIC)     return g_pMountBonus2[cd][1];
        if (type == EF_PARRY)     return g_pMountBonus2[cd][2];
        if (type == EF_RESISTALL) return g_pMountBonus2[cd][3];
        return value;
    }

    for (int i = 0; i < 3; ++i)
    {
        if (item->stEffect[i].cEffect == type)
        {
            int total = item->stEffect[i].cValue;
            if (type == EF_ATTSPEED && total == 1)
                total = 10;
            value += total;
        }
    }

    return value;
}

// ---------------------------------------------------------------------------
// Option colour
// BUG FIX: condições com || mortas corrigidas para refletir as faixas reais:
//   < threshold  → verde   (0xFF99EE99)
//   dentro range → amarelo (0xFFFFFFAA)
//   acima range  → laranja (0xFFFFAA00)
// ---------------------------------------------------------------------------

unsigned int BASE_GetOptionColor(int nPos, unsigned int dwParam, int nValue)
{
    if (nPos >= 64)
    {
        if (nPos != 64 && nPos != 128 && nPos != 192)
            return 0xFF99EE99;

        if (dwParam == 2 || dwParam == 73 || dwParam == 67)
        {
            if (nValue < 45)              return 0xFF99EE99;
            if (nValue <= 54)             return 0xFFFFFFAA;
            return 0xFFFFAA00;
        }
        if (dwParam == 60 || dwParam == 68)
        {
            if (nValue < 20)              return 0xFF99EE99;
            if (nValue <= 24)             return 0xFFFFFFAA;
            return 0xFFFFAA00;
        }
        if (dwParam == 26)
        {
            if (nValue < 21)              return 0xFF99EE99;
            if (nValue <= 24)             return 0xFFFFFFAA;
            return 0xFFFFAA00;
        }
        if (dwParam == 74)
        {
            if (nValue < 21)              return 0xFF99EE99;
            if (nValue <= 24)             return 0xFFFFFFAA;
            return 0xFFFFAA00;
        }

        return 0xFF99EE99;
    }

    if (dwParam == 60 || dwParam == 68)
    {
        if (nPos == 2)
        {
            if (nValue < 12)              return 0xFF99EE99;
            if (nValue <= 14)             return 0xFFFFFFAA;
            return 0xFFFFAA00;
        }
        if (nValue >= 6)
            return (nValue == 6) ? 0xFFFFFFAA : 0xFFFFAA00;

        return 0xFF99EE99;
    }

    if (dwParam == 42 || dwParam == 71)
    {
        if (nValue < 50)              return 0xFF99EE99;
        if (nValue < 60)              return 0xFFFFFFAA;
        return 0xFFFFAA00;
    }

    if (dwParam == 26)
    {
        if (nValue < 12)   return 0xFF99EE99;
        if (nValue == 12)  return 0xFFFFFFAA;
        return 0xFFFFAA00;
    }

    if (dwParam == 74)
    {
        if (nValue < 12)   return 0xFF99EE99;
        if (nValue == 12)  return 0xFFFFFFAA;
        return 0xFFFFAA00;
    }

    if (dwParam == 3 || dwParam == 53 || dwParam == 72)
    {
        if (nPos == 16)
        {
            if (nValue < 30)   return 0xFF99EE99;
            if (nValue == 30)  return 0xFFFFFFAA;
            return 0xFFFFAA00;
        }
        if (nValue < 15)   return 0xFF99EE99;
        if (nValue == 15)  return 0xFFFFFFAA;
        return 0xFFFFAA00;
    }

    if (dwParam == 2 || dwParam == 73 || dwParam == 67)
    {
        if (nPos == 32)
        {
            if (nValue < 24)              return 0xFF99EE99;
            if (nValue <= 30)             return 0xFFFFFFAA;
            return 0xFFFFAA00;
        }
        if (nValue < 18)   return 0xFF99EE99;
        if (nValue == 18)  return 0xFFFFFFAA;
        return 0xFFFFAA00;
    }

    if (dwParam == 4 || dwParam == 45 || dwParam == 69)
    {
        if (nValue < 40)   return 0xFF99EE99;
        if (nValue == 40)  return 0xFFFFFFAA;
        return 0xFFFFAA00;
    }

    return 0xFF99EE99;
}

// ---------------------------------------------------------------------------
// Item amount / effect CRUD
// ---------------------------------------------------------------------------

void BASE_SetItemAmount(STRUCT_ITEM* item, int amount)
{
    if (item->sIndex == 419 || item->sIndex == 420 ||
        item->sIndex == 412 || item->sIndex == 413 ||
        (item->sIndex >= 2390 && item->sIndex <= 2418))
    {
        BASE_RemoveEffect(item, EF_UNIQUE);
    }

    BASE_ChangeOrAddEffectValue(item, EF_AMOUNT, amount);
}

// ---------------------------------------------------------------------------
// Mob ability helpers
// ---------------------------------------------------------------------------

int BASE_GetMobAbility(STRUCT_MOB* mob, char Type)
{
    int value = 0;

    if (Type == 27)
    {
        value = BASE_GetMaxAbility(mob, Type);
        if (value < 2 && mob->Class == 3 && (mob->LearnedSkill[0] & 0x80000))
            value = 2;
        return value;
    }

    int nUnique[16]{};

    for (int i = 0; i < 16; ++i)
    {
        if (!mob->Equip[i].sIndex && i == 7)
            continue;

        if (i >= 1 && i <= 5)
            nUnique[i] = g_pItemList[mob->Equip[i].sIndex].nUnique;

        if ((Type == 2 && i == 6) || (Type == 60 || i == 7))
            continue;

        if (i == 7 && Type == 2)
        {
            const int ldam = BASE_GetItemAbility(&mob->Equip[6], 73) + BASE_GetItemAbility(&mob->Equip[6], Type);
            const int rdam = BASE_GetItemAbility(&mob->Equip[7], 73) + BASE_GetItemAbility(&mob->Equip[7], Type);

            const int lidx = mob->Equip[6].sIndex;
            const int ridx = mob->Equip[7].sIndex;

            int ltype = 0;
            if (lidx > 0 && lidx < 6500) ltype = g_pItemList[lidx].nUnique;

            int rtype = 0;
            if (ridx > 0 && ridx < 6500) rtype = g_pItemList[ridx].nUnique;

            if (!ltype || !rtype)
            {
                value += (ldam <= rdam) ? rdam : ldam;
            }
            else if (ltype == 47 && rtype == 45)
            {
                value += ldam;
            }
            else
            {
                int multi = (ltype == rtype) ? 50 : 30;

                if (!mob->Class && mob->LearnedSkill[0] & 0x200)   multi += 15;
                if (mob->Class == 3 && mob->LearnedSkill[0] & 0x400) multi += 10;

                value += (ldam <= rdam)
                    ? multi * ldam / 100 + rdam
                    : multi * rdam / 100 + ldam;
            }
        }
        else
        {
            value += BASE_GetItemAbility(&mob->Equip[i], Type);
        }
    }

    if (value < 0) value = 0;

    return value;
}

int BASE_GetMaxAbility(STRUCT_MOB* mob, char Type)
{
    int value = 0;

    for (int i = 0; i < 16; ++i)
    {
        if (mob->Equip[i].sIndex)
        {
            const int tvalue = BASE_GetItemAbility(&mob->Equip[i], Type);
            if (tvalue > value) value = tvalue;
        }
    }

    return value;
}

// ---------------------------------------------------------------------------
// Chat / guild name validation
// ---------------------------------------------------------------------------

char BASE_CheckChatValid(const char* Chat)
{
    return 1;
}

char CheckGuildName(const char* GuildName, bool bSubguild)
{
    const int nLen = (int)strlen(GuildName);

    if (!bSubguild)
    {
        for (int i = 0; i < nLen; ++i)
        {
            if (GuildName[i] == ' ' || GuildName[i] == '_')
                return 0;
        }
    }

    if (nLen < 2)
        return 1;

    for (int i = 1; i < nLen; ++i)
        if (GuildName[i] == (char)-95 && GuildName[i - 1] == (char)-95)
            return 0;

    return 1;
}

// ---------------------------------------------------------------------------
// Hit-position helpers
// ---------------------------------------------------------------------------

void BASE_GetHitPosition(int sx, int sy, int* tx, int* ty, char* pHeight, int MH)
{
    if ((sx == *tx && sy == *ty) || !sx || !sy || !*tx || !*ty)
        return;

    const int dx = (sx <= *tx) ? *tx - sx : sx - *tx;
    const int dy = (sy <= *ty) ? *ty - sy : sy - *ty;
    const int dis = BASE_GetDistance(sx, sy, *tx, *ty);

    if (dis <= 0) return;

    if (dis > 30)
    {
        *tx = 0;
        *ty = 0;
        return;
    }

    if (dx > dy)
    {
        if (*tx == sx) return;

        const int a = 1000 * (*ty - sy) / (*tx - sx);
        const int b = 1000 * sy - sx * a;
        const int dir = (sx >= *tx) ? -1 : 1;
        const int sxa = dir + sx;

        int This = pHeight[sxa + g_HeightWidth * ((b + sxa * a) / 1000 - g_HeightPosY) - g_HeightPosX];
        if (This == 127) { *tx = 0; *ty = 0; return; }

        int leng = dx;
        for (int x = sxa; x != *tx; x += dir)
        {
            if (x != sxa)
            {
                const int Last = This;
                This = pHeight[x + g_HeightWidth * ((b + x * a) / 1000 - g_HeightPosY) - g_HeightPosX];
                if (This == 127) { *tx = 0; *ty = 0; return; }
                if (This > MH + Last || This < Last - MH)
                {
                    *tx = x;
                    *ty = (b + x * a) / 1000;
                    return;
                }
                if (--leng < 1) return;
            }
            else if (--leng < 1)
            {
                return;
            }
        }
        return;
    }

    if (*ty == sy) return;

    const int a = 1000 * (*tx - sx) / (*ty - sy);
    const int b = 1000 * sx - sy * a;
    const int dir = (sy >= *ty) ? -1 : 1;
    const int sya = dir + sy;

    int This = pHeight[(b + sya * a) / 1000 + g_HeightWidth * (sya - g_HeightPosY) - g_HeightPosX];
    if (This == 127) { *tx = 0; *ty = 0; return; }

    int leng = dy;
    for (int y = sya; y != *ty; y += dir)
    {
        if (y != sya)
        {
            const int xp = (b + y * a) / 1000;
            const int Last = This;
            This = pHeight[xp + g_HeightWidth * (y - g_HeightPosY) - g_HeightPosX];
            if (This == 127) { *tx = 0; *ty = 0; return; }
            if (This > MH + Last || This < Last - MH)
            {
                *tx = xp;
                *ty = y;
                return;
            }
            if (--leng < 1) return;
        }
        else if (--leng < 1)
        {
            return;
        }
    }
}

int BASE_Get3DTo2DPos(float fX, float fY, float fZ, int* pX, int* pY)
{
    D3DXVECTOR3 vecPos = { fX, fY, fZ };
    D3DXVECTOR3 vTemp, vPosTransformed;

    D3DXVec3TransformCoord(&vTemp, &vecPos, &g_pDevice->m_matView);
    D3DXVec3TransformCoord(&vPosTransformed, &vTemp, &g_pDevice->m_matProj);

    if (vPosTransformed.z < 0.0f || vPosTransformed.z >= 1.0f)
        return 0;

    const int vPosInX = (int)(((vPosTransformed.x + 1.0f) * (float)(g_pDevice->m_dwScreenWidth - g_pDevice->m_nWidthShift)) / 2.0f);
    const int vPosInY = (int)(((-vPosTransformed.y + 1.0f) * (float)(g_pDevice->m_dwScreenHeight - g_pDevice->m_nHeightShift)) / 2.0f);

    if (vPosInX <= 0 || vPosInX >= (int)(g_pDevice->m_dwScreenWidth - g_pDevice->m_nWidthShift) ||
        vPosInY <= 0 || vPosInY >= (int)(g_pDevice->m_dwScreenHeight - g_pDevice->m_nHeightShift))
    {
        return 0;
    }

    *pX = vPosInX;
    *pY = vPosInY;
    return 1;
}

// ---------------------------------------------------------------------------
// Critical-hit progress
// ---------------------------------------------------------------------------

int BASE_GetDoubleCritical(STRUCT_MOB* mob, unsigned short* sProgress,
    unsigned short* cProgress, char* bDoubleCritical)
{
    *bDoubleCritical = 0;
    if (!cProgress) return 0;

    if ((int)*cProgress >= 1024) *cProgress %= 1024;
    if (sProgress && (int)*sProgress >= 1024) *sProgress %= 1024;

    const int hitvalue[2] =
    {
        100 * (((int)(unsigned char)mob->CurrentScore.AttackRun >> 4) - 5),
        4 * (unsigned char)mob->Critical
    };

    if (sProgress && cProgress && *cProgress != *sProgress)
    {
        if (*sProgress > (int)*cProgress) *cProgress = *sProgress;
        if (*sProgress < (int)*cProgress)
        {
            if (*sProgress + 5 < *cProgress)
            {
                *cProgress = *sProgress;
                return 0;
            }
            *sProgress = *cProgress;
        }
    }

    const int value = g_pHitRate[*cProgress];

    for (int i = 0; i < 2; ++i)
    {
        int bit = 0;
        if (!i && value < hitvalue[0])              bit = 1;
        if (i == 1 && 1000 - value < hitvalue[1])   bit = 1;
        *bDoubleCritical |= bit << i;
    }

    if (sProgress) ++*sProgress;
    ++*cProgress;

    return 1;
}

void BASE_GetHitPosition2(int sx, int sy, int* tx, int* ty, char* pHeight, int MH)
{
    if ((sx == *tx && sy == *ty) || !sx || !sy || !*tx || !*ty)
        return;

    const int dx = (sx <= *tx) ? *tx - sx : sx - *tx;
    const int dy = (sy <= *ty) ? *ty - sy : sy - *ty;
    int       This = pHeight[sx + g_HeightWidth * (sy - g_HeightPosY) - g_HeightPosX];

    if (dx > dy)
    {
        const int dir = (sx >= *tx) ? -1 : 1;
        int       leng = dx;

        for (int x = sx; x - dir != *tx; x += dir)
        {
            if (x == sx)
            {
                if (--leng < 0) return;
                continue;
            }

            const int Last = This;
            const int a = 1000 * (*ty - sy) / (*tx - sx);
            This = pHeight[x + g_HeightWidth * ((1000 * sy - sx * a + x * a) / 1000 - g_HeightPosY) - g_HeightPosX];

            if (This > MH + Last || This < Last - MH)
            {
                *tx = 0; *ty = 0;
                return;
            }
            if (--leng < 0) return;
        }
        return;
    }

    const int dir = (sy >= *ty) ? -1 : 1;
    int       leng = dy;

    for (int y = sy; y - dir != *ty; y += dir)
    {
        if (y == sy)
        {
            if (--leng < 0) return;
            continue;
        }

        const int xt = 1000 * (*tx - sx) / (*ty - sy);
        const int xp = (1000 * sx - sy * xt + y * xt) / 1000;
        const int Last = This;

        if (y - g_HeightPosY < 0 || y - g_HeightPosY >= 256 ||
            xp - g_HeightPosX < 0 || xp - g_HeightPosX >= 256)
        {
            *tx = 0; *ty = 0;
            return;
        }

        This = pHeight[xp + g_HeightWidth * (y - g_HeightPosY) - g_HeightPosX];

        if (y == sy) { --leng; continue; }

        if (This > MH + Last || This < Last - MH)
        {
            *tx = 0; *ty = 0;
            return;
        }
        if (--leng < 0) return;
    }
}

// ---------------------------------------------------------------------------
// Bit / height-map utilities
// ---------------------------------------------------------------------------

void BASE_SetBit(char* byte, int pos)
{
    byte[pos / 8] |= 1 << (pos % 8);
}

int BASE_UpdateItem2(int maskidx, int CurrentState, int NextState,
    int xx, int yy, char* pHeight, int rotate, int height)
{
    if (maskidx >= 10 || maskidx < 0)
        return 0;

    for (int y = 0; y <= 5; ++y)
    {
        for (int x = 0; x <= 5; ++x)
        {
            const int xp = xx + x - 2;
            const int yp = yy + y - 2;

            if (xp - g_HeightPosX < 1 || yp - g_HeightPosY < 1 ||
                xp - g_HeightPosX > g_HeightWidth - 2 ||
                yp - g_HeightPosY > g_HeightHeight - 2)
            {
                break;
            }

            if (g_pGroundMask[maskidx][rotate][y][x])
                pHeight[xp + g_HeightWidth * (yp - g_HeightPosY) - g_HeightPosX] = height;
        }
    }

    return 1;
}

int BASE_GetMeshIndex(short sIndex)
{
    STRUCT_ITEM item{};
    item.sIndex = sIndex;

    const int nPos = BASE_GetItemAbility(&item, 17);
    const int nClassType = BASE_GetItemAbility(&item, 18);
    int       nClassIndex = 0;

    if (g_pItemList[sIndex].nIndexMesh >= 40 && g_pItemList[sIndex].nIndexMesh < 50 &&
        (nPos & 4 || nPos & 8 || nPos & 0x10 || nPos & 0x20))
    {
        if (g_pItemList[sIndex].nIndexMesh == 40)
            return 0;

        switch (nClassType)
        {
        case 1: nClassIndex = 0; break;
        case 4: nClassIndex = 1; break;
        case 2: nClassIndex = 2; break;
        case 8: nClassIndex = 3; break;
        }

        // BUG FIX (original): nPos era sobrescrito com valor de bit (0-3),
        // perdendo o valor original. Usamos nPosBit separado.
        int nPosBit = 0;
        if (nPos & 4)    nPosBit = 0;
        else if (nPos & 8)    nPosBit = 1;
        else if (nPos & 0x10) nPosBit = 2;
        else if (nPos & 0x20) nPosBit = 3;

        return nPosBit + 4 * (g_pItemList[sIndex].nIndexMesh + nClassIndex - 41) + 1401;
    }

    switch (nClassType)
    {
    case 1: nClassIndex = 0;   break;
    case 2: nClassIndex = 200; break;
    case 4: nClassIndex = 20;  break;
    case 8: nClassIndex = 220; break;
    }

    if (nClassType <= 8)
    {
        if (nPos & 2)    return g_pItemList[sIndex].nIndexMesh + nClassIndex + 1001;
        if (nPos & 4)    return g_pItemList[sIndex].nIndexMesh + nClassIndex + 1041;
        if (nPos & 8)    return g_pItemList[sIndex].nIndexMesh + nClassIndex + 1081;
        if (nPos & 0x10) return g_pItemList[sIndex].nIndexMesh + nClassIndex + 1121;
        if (nPos & 0x20) return g_pItemList[sIndex].nIndexMesh + nClassIndex + 1161;
    }

    return g_pItemList[sIndex].nIndexMesh;
}

// ---------------------------------------------------------------------------
// Refinement / sanc helpers
// ---------------------------------------------------------------------------

bool BASE_CanRefine(STRUCT_ITEM* item)
{
    for (auto i : item->stEffect)
    {
        if (i.cEffect == 0 || i.cEffect == EF_SANC ||
            (i.cEffect >= EF_COLOR0 && i.cEffect <= EF_COLOR9))
        {
            return true;
        }
    }
    return false;
}

int IsPassiveSkill(int nSkillIndex)
{
    bool freeskill = (nSkillIndex == 5054);

    if (!freeskill)
    {
        if (nSkillIndex >= 5400)
            nSkillIndex -= 5200;
        else if (nSkillIndex >= 5000 && nSkillIndex <= 5102)
            nSkillIndex -= 5000;

        if (nSkillIndex >= 0 && nSkillIndex < MAX_SPELL_LIST)
        {
            if (g_pSpell[nSkillIndex].Passive == 1)
                return 1;
        }
    }

    return 0;
}

bool BASE_HasSancAdd(STRUCT_ITEM* item)
{
    for (auto i : item->stEffect)
    {
        if (i.cEffect == 43 || (i.cEffect >= 115 && i.cEffect <= 126))
            return true;
    }
    return false;
}

bool BASE_HasSancAdd(STRUCT_BONUSEFFECT effect)
{
    return (effect.cEffect == 43 || (effect.cEffect >= 115 && effect.cEffect <= 126));
}

int BASE_GetSancEffValue(STRUCT_ITEM* item)
{
    for (auto i : item->stEffect)
    {
        if (i.cEffect == EF_SANC || (i.cEffect >= EF_STARTCOL && i.cEffect <= EF_MAXCOL))
            return i.cValue;
    }
    return 0;
}

int BASE_GetItemSancSuccess(STRUCT_ITEM* item)
{
    const int idx = item->sIndex;

    if ((idx >= 2330 && idx < 2390) ||
        (idx >= 3200 && idx < 3300) ||
        (idx >= 3980 && idx < 4000))
    {
        return 0;
    }

    const int sanc = BASE_GetSancEffValue(item);
    return (sanc <= 210) ? sanc / 10 : 0;
}

// ---------------------------------------------------------------------------
// Effect CRUD
// ---------------------------------------------------------------------------

int BASE_GetEffectValue(STRUCT_ITEM* item, int effect)
{
    for (auto i : item->stEffect)
    {
        if (i.cEffect == effect)
            return i.cValue;
    }
    return 0;
}

void BASE_ChangeOrAddEffectValue(STRUCT_ITEM* item, int effect, int value)
{
    for (auto& i : item->stEffect)
    {
        if (i.cEffect == effect)
        {
            i.cValue = static_cast<unsigned char>(value);
            return;
        }
    }

    // If the item doesn't have the effect, search for an empty slot to insert it.
    for (auto& i : item->stEffect)
    {
        if (i.cEffect == 0)
        {
            i.cEffect = static_cast<unsigned char>(effect);
            i.cValue = static_cast<unsigned char>(value);
            return;
        }
    }
}

void BASE_RemoveEffect(STRUCT_ITEM* item, int effect)
{
    for (auto& i : item->stEffect)
    {
        if (i.cEffect == effect)
        {
            i.cEffect = 0;
            i.cValue = 0;
        }
    }
}