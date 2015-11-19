#ifndef __IB_SVG_H__
#define __IB_SVG_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_platform.h"

namespace ib{
  unsigned char* rasterize_svg_file(const char *path, int size);
}


#endif
