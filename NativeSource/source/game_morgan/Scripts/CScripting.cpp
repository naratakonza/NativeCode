#include "../../../Main.h"
#include "CScripting.h"

std::uint32_t dwScmOpcodeDebug = 0;
CRunningScript* CScripting::gst = nullptr;
char CScripting::ScriptBuf[512]{};
std::uint32_t* CScripting::pdwParamVars[18]{};

static bool executingScriptCommand = false;

void CScripting::Init()
{
    if (!pthread_main_np() || CScripting::gst) {
        return;
    }

    if (!CRunningScript::Initialise()) {
        CExceptions::Print(OBFUSCATE("CScripting Init"), OBFUSCATE("GTA script dispatcher unavailable"));
        return;
    }

    static CRunningScript script{};
    CScripting::gst = &script;
}

std::uint8_t CScripting::Execute()
{
    if (!pthread_main_np() || !CScripting::gst || executingScriptCommand) {
        return 0;
    }

    CScripting::gst->m_pPCPointer = reinterpret_cast<std::uint8_t*>(CScripting::ScriptBuf);
    CScripting::gst->m_bCmpFlag = 0;
    executingScriptCommand = true;

    try
    {
        CRunningScript::ProcessOneCommand(CScripting::gst);
    }
    catch (const std::exception& error)
    {
        executingScriptCommand = false;
        char context[32];
        std::snprintf(context, sizeof(context), OBFUSCATE("opcode 0x%04X"), static_cast<unsigned int>(dwScmOpcodeDebug));
        CExceptions::Print(context, error.what());
        throw;
    }
    catch (...)
    {
        executingScriptCommand = false;
        CExceptions::Print(OBFUSCATE("CScripting Execute"), OBFUSCATE("unknown C++ exception"));
        throw;
    }

    executingScriptCommand = false;
    return CScripting::gst->m_bCmpFlag;
}

int CScripting::ScriptCommand(const SCRIPT_COMMAND* pScriptCommand, ...)
{
    if (!pthread_main_np() || !CScripting::gst || executingScriptCommand || !pScriptCommand) {
        return 0;
    }

    const auto opcode = pScriptCommand->OpCode;
    if ((opcode & 0x7FFF) > CRunningScript::MaxOpcode || !std::memchr(pScriptCommand->Params, '\0', sizeof(pScriptCommand->Params))) {
        return 0;
    }

    *CScripting::gst = {};
    CScripting::gst->m_pBaseAddressOfThisScript = reinterpret_cast<std::uint8_t*>(CScripting::ScriptBuf);
    std::memset(CScripting::ScriptBuf, 0, sizeof(CScripting::ScriptBuf));
    std::memcpy(CScripting::ScriptBuf, &opcode, sizeof(opcode));
    std::size_t bufPos = sizeof(opcode);
    std::uint16_t varPos = 0;

    const auto append = [&](const void* data, std::size_t size)
    {
        if (size > sizeof(CScripting::ScriptBuf) - bufPos) {
            return false;
        }

        std::memcpy(CScripting::ScriptBuf + bufPos, data, size);
        bufPos += size;
        return true;
    };

    const auto parameter = [&](std::uint8_t tag, const void* data, std::size_t size)
    {
        return append(&tag, sizeof(tag)) && append(data, size);
    };

    va_list args;
    va_start(args, pScriptCommand);
    bool encoded = true;

    for (const char* p = pScriptCommand->Params; *p && encoded; ++p)
    {
        switch (*p)
        {
            case 'i':
            case 'd':
            case 'b': {
                const int argument = va_arg(args, int);
                const std::int32_t value = *p == 'b' ? argument != 0 : argument;
                encoded = parameter(0x01, &value, sizeof(value));
                break;
            }

            case 'v':
            case 'p': {
                auto* value = va_arg(args, std::uint32_t*);
                if (!value || varPos == sizeof(CScripting::pdwParamVars) / sizeof(CScripting::pdwParamVars[0])) {
                    encoded = false;
                    break;
                }

                CScripting::pdwParamVars[varPos] = value;
                CScripting::gst->m_nLocals[varPos] = *value;
                encoded = parameter(0x03, &varPos, sizeof(varPos));
                ++varPos;
                break;
            }

            case 'f': {
                const float value = static_cast<float>(va_arg(args, double));
                encoded = parameter(0x06, &value, sizeof(value));
                break;
            }

            case 's': {
                const auto* value = va_arg(args, const char*);
                const auto length = value ? ::strnlen(value, 256) : 256;
                const auto size = static_cast<std::uint8_t>(length);
                encoded = length < 256 && parameter(0x0E, &size, sizeof(size)) && append(value, length);
                break;
            }

            case 'z': {
                const std::uint8_t terminator = 0;
                encoded = append(&terminator, sizeof(terminator));
                break;
            }

            default: {
                encoded = false;
                break;
            }
        }
    }

    va_end(args);
    if (!encoded) {
        return 0;
    }

    dwScmOpcodeDebug = opcode;
    const auto result = CScripting::Execute();

    for (std::uint16_t i = 0; i < varPos; ++i)
    {
        *CScripting::pdwParamVars[i] = CScripting::gst->m_nLocals[i];
    }

    return result;
}