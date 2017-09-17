#include "modelling/BB.hpp"

BB::BB()
{
	reset();
}

BB::~BB()
{
}

void BB::reset()
{
	x0 = y0 = z0 =  99999.0f;
	x1 = y1 = z1 = -99999.0f;
	center[0] = center[1] = center[2] = 0.0f;
}
void BB::updateCenter()
{
	center[0] = (x1 + x0) / 2.0f;
	center[1] = (y1 + y0) / 2.0f;
	center[2] = (z1 + z0) / 2.0f;
}