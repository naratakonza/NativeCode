#include "../../Main.h"
#include "CHud.h"

#include <mach-o/dyld.h>

int16_t& CHud::m_ItemToFlash = *reinterpret_cast<int16_t*>(SA_ADDR(0x7ACF00) + _dyld_get_image_vmaddr_slide(0));