#pragma once

#include <cstdint>
#include <unordered_map>

#include "../../Texts/CTextLabel.h"
#include "../../../Enums/eMaxItem.h"

class CTextLabelPool
{
private:
    std::unordered_map<uint16_t, CTextLabel*> m_pLabels;
    std::unordered_map<uint16_t, bool> m_bLabelsSlot;

public:
    CTextLabelPool();
    ~CTextLabelPool();
    void Draw();
    void Process() { Draw(); }
    void DeleteAll();

    bool Add(int labelID, char* szText, uint32_t dwColor, CVector vecPos, float fDrawDistance, bool bUseLOS, uint16_t attachedToPlayerID, uint16_t attachedToVehicleID);
    bool Delete(int labelID);

    CTextLabel* GetTextLabel(uint16_t id) {
        auto it = CTextLabelPool::m_pLabels.find(id);
        if (it != CTextLabelPool::m_pLabels.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool GetTextLabelInSlot(uint16_t id) {
        auto it = CTextLabelPool::m_bLabelsSlot.find(id);
        if (it != CTextLabelPool::m_bLabelsSlot.end()) {
            return it->second;
        }

        return false;
    }
};