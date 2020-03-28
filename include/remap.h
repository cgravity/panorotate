#include "image.h"
#include "custom_math.h"

// current best quality method
void remap_full3(
    Image<RGBAF>& onto, 
    const Image<RGBAF>& from, 
    Mat3 rot = ident(),
    const double s = 0.4);

// single sample per pixel to produce a quick result for preview
void remap_fast(Image<RGBAF>& onto, const Image<RGBAF>& from, Mat3 rot);

// original implementation -- only included still for quality comparison
void remap_full1(
    Image<RGBAF>& onto, 
    const Image<RGBAF>& from, 
    Mat3 rot = ident());

