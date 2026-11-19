#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>

struct DeathWindowStruct
{
    std::string victimName;
    uint32_t victimColor;
    std::string attackerName;
    uint32_t attackerColor;
    uint8_t deathReason;
    uint32_t fontcolor;
};

class CDeathWindow
{
public:
    CDeathWindow();
    ~CDeathWindow();

    void AddEntry(const std::string& victimName, uint32_t victimColor, const std::string& attackerName, uint32_t attackerColor, uint8_t deathReason, uint32_t fontcolor);
    void Render();

private:
    const char* GetWeaponSpriteId(uint8_t byteWeaponID);

private:
    std::list<std::shared_ptr<DeathWindowStruct>> m_pDeathWindow;

};