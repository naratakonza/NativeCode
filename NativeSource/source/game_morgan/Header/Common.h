#pragma once

#define PADDING(x,y) uint8_t x[y]

#define ARRAY_SIZE(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#define ARRAY_VALIDATE(_Array, i) ((i) >= 0 && static_cast<decltype(sizeof(_Array))>(i) < ARRAY_SIZE(_Array))

#define SAFE_DELETE(p) { if (p) { delete (p); (p) = nullptr; } }
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p); (p) = nullptr; } }
#define SAFE_RELEASE(p)	{ if (p) { (p)->Release(); (p) = nullptr; } }

#define IS_VEHICLE_MOD(x) (x >= 1000 && x <= 1193)