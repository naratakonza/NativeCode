#include "../../../../../Main.h"
#include "CTextLabelPool.h"
#include "../../Guis/CGui.h"

#include <new>

CTextLabelPool::CTextLabelPool()
{
    CNecessary::Log(OBFUSCATE("CTextLabelPool Initialise"));
}

CTextLabelPool::~CTextLabelPool()
{
    CNecessary::Log(OBFUSCATE("CTextLabelPool Uninitialise"));
    CTextLabelPool::DeleteAll();
}

void CTextLabelPool::DeleteAll()
{
    for (auto labelIter = CTextLabelPool::m_pLabels.begin(); labelIter != CTextLabelPool::m_pLabels.end();)
    {
        SAFE_DELETE(labelIter->second);
        labelIter = CTextLabelPool::m_pLabels.erase(labelIter);
    }

    CTextLabelPool::m_bLabelsSlot.clear();
}

void CTextLabelPool::Draw()
{
    for (const auto& labelPair : CTextLabelPool::m_pLabels)
    {
        CTextLabel* pLabel = labelPair.second;
        if (pLabel)
        {
            pLabel->Render();
        }
    }
}

bool CTextLabelPool::Add(int labelID, char* szText, uint32_t dwColor, CVector vecPos, float fDrawDistance, bool bUseLOS, uint16_t attachedToPlayerID, uint16_t attachedToVehicleID)
{
    if (labelID < 0 || labelID >= eMaxItem::MAX_TEXT_LABELS + eMaxItem::MAX_PLAYER_TEXT_LABELS + 2) {
        return false;
    }

    CTextLabelPool::Delete(labelID);

    CTextLabel* newLabel = new (std::nothrow) CTextLabel(labelID, szText, dwColor, vecPos, fDrawDistance, bUseLOS, attachedToPlayerID, attachedToVehicleID);
    if (!newLabel) {
        return false;
    }

    CTextLabelPool::m_pLabels.emplace(labelID, newLabel);
    CTextLabelPool::m_bLabelsSlot[labelID] = true;

    return true;
}

bool CTextLabelPool::Delete(int labelID)
{
    if (labelID < 0 || labelID >= eMaxItem::MAX_TEXT_LABELS + eMaxItem::MAX_PLAYER_TEXT_LABELS + 2) {
        return false;
    }

    auto labelIter = CTextLabelPool::m_pLabels.find(labelID);
    if (labelIter != CTextLabelPool::m_pLabels.end())
    {
        SAFE_DELETE(labelIter->second);
        CTextLabelPool::m_pLabels.erase(labelIter);
        CTextLabelPool::m_bLabelsSlot[labelID] = false;

        return true;
    }

    return false;
}