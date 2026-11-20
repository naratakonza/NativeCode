#include "CHeadMove.h"

bool CHeadMove::m_bEnableHeadMove = false;

void CHeadMove::SetHeadMove(bool Enable)
{
    CHeadMove::m_bEnableHeadMove = Enable;
}

bool CHeadMove::GetHeadMove()
{
    return CHeadMove::m_bEnableHeadMove;
}