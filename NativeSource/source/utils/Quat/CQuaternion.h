#pragma once

#include "../../game_morgan/Core/CMatrix.h"

class CQuaternion
{
public:
	CQuaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {};
	CQuaternion(float _x, float _y, float _z, float _w) : w(_w), x(_x), y(_y), z(_z) {};

	void Set(float _x, float _y, float _z, float _w)
	{
		w = _w;
		x = _x;
		y = _y;
		z = _z;
	}

	void Set(CQuaternion q)
	{
		w = q.w;
		x = q.x;
		y = q.y;
		z = q.z;
	}

	void SetFromMatrix(CMatrix mat);
	void GetMatrix(CMatrix* mat);
	void Normalize();
	void Slerp(CQuaternion *pQ1, CQuaternion *pQ2, float t);

public:
	float w;
	float x;
	float y;
	float z;
};