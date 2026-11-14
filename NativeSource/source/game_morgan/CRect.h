#pragma once

class CRect
{
public:
	CRect(float left, float bottom, float right, float top) : X1(left), Y1(bottom), X2(right), Y2(top) {};

	CRect()
	{
		X1 = Y1 = X2 = Y2 = 0.0f;
	}

	CRect(float f)
	{
		X1 = Y1 = X2 = Y2 = f;
	}
	
	
	float X1; 	// X1 left
	float Y1; 	// Y1 bottom
	float X2; 	// X2 right
	float Y2;	// Y2 top

	void Restrict(CRect* rect)
    {
        if (rect->X1 < X1)
            X1 = rect->X1;
        if (rect->X2 > X2)
            X2 = rect->X2;
        if (rect->Y2 < Y2)
            Y2 = rect->Y2;
        if (rect->Y1 > Y1)
            Y1 = rect->Y1;
    }

    void StretchToPoint(float x, float y)
    {
        if (x < X1)
            X1 = x;
        if (x > X2)
            X2 = x;
        if (y < Y2)
            Y2 = y;
        if (y > Y1)
            Y1 = y;
    }

    bool IsCircleInside(float* point, float radius)
    {
        return X1 - radius <= point[0] && radius + X2 >= point[0] && Y2 - radius <= point[1] && radius + Y1 >= point[1];
    }

    bool PtInRect(float x, float y) const
    {
        return (x >= X1 && x <= X2 && y >= Y1 && y <= Y2);
    }

    void Reset()
    {
        X1 = 0.0f;
        Y1 = 0.0f;
        X2 = 0.0f;
        Y2 = 0.0f;
    }

    void FixIncorrectTopX1()
    {
        if (X1 > X2)
            X1 = X2 - 1.0f;
        if (Y2 > Y1)
            Y2 = Y1 - 1.0f;
    }
};
static_assert(sizeof(CRect) == 0x10);
