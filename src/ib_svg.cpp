#include "ib_svg.h"

#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"


unsigned char* ib::rasterize_svg_file(const char *path, int size) {
  unsigned char *result = nullptr;
  NSVGimage *svg_image = nullptr;
  NSVGrasterizer *rasterizer= nullptr;
  double scale;

  svg_image = nsvgParseFromFile(path, "px", 96.0f);
  if(svg_image == 0) goto finally;
  rasterizer = nsvgCreateRasterizer();
  if(rasterizer == 0) goto finally;

  scale = (double)size / (double)svg_image->width;
  result = new unsigned char[size * size * 4];
  nsvgRasterize(rasterizer, svg_image, 0, 0, scale, result, size, size, size*4);
finally:
  if(rasterizer != nullptr) nsvgDeleteRasterizer(rasterizer);
  if(svg_image != nullptr) nsvgDelete(svg_image);
  return result;
}
