#pragma once

class CRunningScript;

using ScriptCommandHandler = std::int8_t (*)(CRunningScript*, int);

struct ScriptCommandSlot
{
    ScriptCommandHandler handler;
    std::int64_t thisAdjustment;
};
static_assert(sizeof(ScriptCommandSlot) == 16);

class CRunningScript
{
public:
    CRunningScript* m_pNext;
    CRunningScript* m_pPrevious;
    char m_szScriptName[8];
    std::uint8_t* m_pBaseAddressOfThisScript;
    std::uint8_t* m_pPCPointer;
    std::uint8_t* m_pPCStack[8];
    std::uint16_t m_nStackDepth;
    std::uint8_t pad01[2];
    std::uint32_t m_nLocals[42];
    std::uint8_t m_bActive;
    std::uint8_t m_bCmpFlag;
    std::uint8_t m_bIsThisAMissionScript;
    std::uint8_t m_bIsThisAStreamedScript;
    std::uint8_t m_bIsThisAMiniGameScript;
    std::int8_t m_nScriptBrainType;
    std::uint8_t pad02[2];
    std::int32_t m_nActivateTime;
    std::uint16_t m_nAndOrState;
    std::uint8_t m_bNotForLatestExpression;
    std::uint8_t m_bDeathArrestEnabled;
    std::uint8_t m_bDeathArrestExecuted;
    std::uint8_t pad04[3];
    std::uint32_t m_nEndOfScriptedCutscenePC;
    std::uint8_t m_bThisMustBeTheOnlyMissionRunning;
    std::uint8_t pad03[3];

public:
    static constexpr std::uint16_t MaxOpcode = 0xA8B;

    static bool Initialise();
    static void ProcessOneCommand(CRunningScript* thiz);
};

static_assert(sizeof(CRunningScript) == 0x130);
static_assert(offsetof(CRunningScript, m_pPCPointer) == 0x20);
static_assert(offsetof(CRunningScript, m_nLocals) == 0x6C);
static_assert(offsetof(CRunningScript, m_bCmpFlag) == 0x115);
static_assert(offsetof(CRunningScript, m_bNotForLatestExpression) == 0x122);