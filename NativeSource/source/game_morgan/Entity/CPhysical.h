#pragma once

#include "CPhysicalGta.h"

class CPhysical
{
private:
    static float m_fGravity;
public:
    static void Initialise();
    __attribute__((visibility("default"))) static void ProcessControl(CPhysicalGta *thiz);
    static void SetGravity(float fGravity);
    static float GetGravity();
    static void ApplyGravity(CPhysicalGta *thiz);
    static void ApplyForce(CPhysicalGta *thiz, CVector vecForce, CVector vecOffset, bool bValidTorque);
    static void ApplyAirResistance(CPhysicalGta *thiz);
    static void ApplyFriction(CPhysicalGta *thiz);
};