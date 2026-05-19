#pragma once

// ============================================================================
// PACKET VALIDATION LAYER - WYD Client DX9 Security Hardening
// ============================================================================
// Version: 2.0 - Expanded opcode whitelist and bounds
// ============================================================================

#include <windows.h>
#include <cstddef>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define ENABLE_PACKET_VALIDATION 1

// Enable debug logging (logs to OutputDebugString)
#define PACKET_VALIDATION_LOGGING 1

// Maximum acceptable packet size
#define MAX_PACKET_SIZE 65536

// Minimum acceptable packet size (must at least have header)
#define MIN_PACKET_SIZE 12

// Game-specific limits (ajuste conforme seu Basedef.h)
#define MAX_ITEMLIST 4000      // Número máximo de itens no jogo
#define MAX_CARRY 60           // Máximo de itens carregáveis no inventário

// ============================================================================
// VALIDATION LOGGING
// ============================================================================

#ifdef PACKET_VALIDATION_LOGGING
#include <stdio.h>
#define PV_LOG(format, ...) \
        do { \
            char logBuffer[512]; \
            sprintf(logBuffer, "[PacketValidator] " format "\n", ##__VA_ARGS__); \
            OutputDebugStringA(logBuffer); \
        } while(0)
#else
#define PV_LOG(format, ...) ((void)0)
#endif

// ============================================================================
// OPCODE WHITELIST (expandida)
// ============================================================================

enum VALID_OPCODES
{
    // Movimentação e Ações Básicas
    OPCODE_ACTION = 0x102,
    OPCODE_ACTION_STOP = 0x103,
    OPCODE_MESSAGE_CHAT = 0x105,
    OPCODE_MESSAGE_WHISPER = 0x106,
    OPCODE_SET_POSITION = 0x1C1,
    OPCODE_SET_HP_MODE = 0x1D0,

    // Mobs e Combate
    OPCODE_CREATE_MOB = 0x101,
    OPCODE_MOBKILL = 0x16F,
    OPCODE_MOBSKILL = 0x175,

    // Itens e Inventário
    OPCODE_CREATE_ITEM = 0x10A,
    OPCODE_DROP_ITEM = 0x114,
    OPCODE_GET_ITEM = 0x116,
    OPCODE_UPDATE_ITEM = 0x171,
    OPCODE_USE_ITEM = 0x373, // Adicionado (MacroPerga)
    OPCODE_SEND_ITEM = 0x372, // Exemplo

    // NPCs, Lojas e Banca
    OPCODE_SHOP_LIST = 0x17C,
    OPCODE_REQ_SHOP_LIST = 0x27B,
    OPCODE_BUY = 0x17E,
    OPCODE_SELL = 0x17D,
    OPCODE_CLOSE_SHOP = 0x17F,
    OPCODE_DEPOSIT = 0x180,
    OPCODE_WITHDRAW = 0x181,
    OPCODE_SWAP_ITEM = 0x182,
    OPCODE_SHOP_LIST_RMB = 0x28E,
    OPCODE_UNDO_SELL_ITEM = 0x296,

    // Party e Guild
    OPCODE_PARTY = 0x379,
    OPCODE_ADD_PARTY = 0x374,
    OPCODE_REMOVE_PARTY = 0x375,

    // Misc
    OPCODE_LONG_MESSAGE = 0x333,
    OPCODE_REQ_SUMMON = 0x334,
    OPCODE_STANDARD_PARM = 0x338,
    OPCODE_STANDARD_PARM2 = 0x339,
    OPCODE_REMOVE_SERVER = 0x37D,
    OPCODE_REMOVE_SERV_LOG = 0x37E,
    OPCODE_CHAR_LOGIN = 0x37F,
    OPCODE_REPURCHASE = 0x378,
    OPCODE_SET_SHORT_SKILL = 0x292,
    OPCODE_ENCODE = 0x39B,
    OPCODE_EXEC_COMMAND = 0x3A1,
    OPCODE_3A3 = 0x3A3,
    OPCODE_3A7 = 0x3A7,
    OPCODE_3AC = 0x3AC,
    OPCODE_3B2 = 0x3B2,
    OPCODE_3B3 = 0x3B3,
    OPCODE_18B = 0x18B,
    OPCODE_18D = 0x18D,
    OPCODE_19C = 0x19C,
    OPCODE_26E = 0x26E,
    OPCODE_52A = 0x52A,
    OPCODE_7B1 = 0x7B1,
    OPCODE_3E8 = 0x3E8,
};

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

#ifdef ENABLE_PACKET_VALIDATION

inline bool PV_ValidatePacketSize(const MSG_STANDARD* pStd, size_t receivedSize)
{
    if (!pStd) return false;
    if (receivedSize < MIN_PACKET_SIZE || receivedSize > MAX_PACKET_SIZE) return false;
    if (pStd->Size < MIN_PACKET_SIZE || pStd->Size > MAX_PACKET_SIZE) return false;
    if (pStd->Size > receivedSize) PV_LOG("WARNING: Claimed size > received, possible truncation");
    return true;
}

inline bool PV_ValidateOpcode(int opcode)
{
    static const int SAFE_OPCODES[] = {
        // lista completa de opcodes
        0x101,0x102,0x103,0x105,0x106,0x10A,0x114,0x116,0x16F,0x171,
        0x175,0x1C1,0x1D0,0x292,0x333,0x334,0x338,0x339,0x372,0x373,
        0x374,0x375,0x378,0x379,0x37D,0x37E,0x37F,0x39B,0x3A1,0x3A3,
        0x3A7,0x3AC,0x3B2,0x3B3,0x18B,0x18D,0x19C,0x26E,0x52A,0x7B1,
        0x3E8,0x17C,0x27B,0x17D,0x17E,0x17F,0x180,0x181,0x182,0x28E,
        0x296
    };
    const int numOpcodes = sizeof(SAFE_OPCODES) / sizeof(SAFE_OPCODES[0]);
    for (int i = 0; i < numOpcodes; ++i)
        if (SAFE_OPCODES[i] == opcode) return true;
    PV_LOG("Unknown opcode 0x%X - allowing (fail-open)", opcode);
    return true; // fail-open
}

inline bool PV_ValidatePacketType(const MSG_STANDARD* pStd, int expectedOpcode, size_t expectedSize)
{
    if (!pStd || pStd->Type != expectedOpcode) return false;
    if (pStd->Size < expectedSize) return false;
    return true;
}

inline bool PV_ValidateEquipIndex(int index) { return (index >= 0 && index < 16); }
inline bool PV_ValidateCarryIndex(int index) { return (index >= 0 && index < MAX_CARRY); }
inline bool PV_ValidateCargoIndex(int index) { return (index >= 0 && index < 128); }

template<typename T>
inline T* PV_SafeCastPacket(MSG_STANDARD* pStd, size_t minSize)
{
    if (!pStd || pStd->Size < minSize) return nullptr;
    return reinterpret_cast<T*>(pStd);
}

#define PV_VALIDATE_ENTRY(pStd, recvSize) if (!PV_ValidatePacketSize((pStd), (recvSize))) return 1;
#define PV_VALIDATE_PACKET_TYPE(pStd, opcode, type) if (!PV_ValidatePacketType((pStd), (opcode), sizeof(type))) return 1;
#define PV_SAFE_EQUIP(pMobData, index) (PV_ValidateEquipIndex(index) ? &(pMobData)->Equip[index] : nullptr)
#define PV_SAFE_CARRY(pMobData, index) (PV_ValidateCarryIndex(index) ? &(pMobData)->Carry[index] : nullptr)
#define PV_SAFE_CARGO(cargo, index) (PV_ValidateCargoIndex(index) ? &(cargo)[index] : nullptr)

#else
// stubs
#define PV_VALIDATE_ENTRY(pStd, recvSize) ((void)0)
#define PV_VALIDATE_PACKET_TYPE(pStd, opcode, type) ((void)0)
#define PV_SAFE_EQUIP(pMobData, index) (&(pMobData)->Equip[index])
#define PV_SAFE_CARRY(pMobData, index) (&(pMobData)->Carry[index])
#define PV_SAFE_CARGO(cargo, index) (&(cargo)[index])
inline bool PV_ValidatePacketSize(const MSG_STANDARD*, size_t) { return true; }
inline bool PV_ValidateOpcode(int) { return true; }
inline bool PV_ValidateEquipIndex(int) { return true; }
inline bool PV_ValidateCarryIndex(int) { return true; }
inline bool PV_ValidateCargoIndex(int) { return true; }
template<typename T> inline T* PV_SafeCastPacket(MSG_STANDARD* p, size_t) { return reinterpret_cast<T*>(p); }
#endif