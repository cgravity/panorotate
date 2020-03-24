#include <cmath>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>
using namespace std;

#include "math.h"
#include "image.h"

void double_rotate_test(const Image<RGBAF>& src);

int main(int argc, char** argv)
{
    double rx = 0, ry = 0, rz = 0;
    
    if(argc >= 2)
    {
        if(sscanf(argv[1], "%lf", &rx))
            rx = deg2rad(rx);
    }
    
    if(argc >= 3)
    {
        if(sscanf(argv[2], "%lf", &ry))
            ry = deg2rad(ry);
    }
    
    if(argc >= 4)
    {
        if(sscanf(argv[3], "%lf", &rz))
            rz = deg2rad(rz);
    }

 #if 0
    Image<RGBAF> src, dst;
    //if(!load(src, "data/constellations_2048.jpg"))
    //if(!load(src, "data/WI-Capitol-360x180-L.jpg"))

    
    dst.resize(src.width, src.height);
    remap_full2(dst, src, rotZ(rz)*rotY(ry)*rotX(rx));
    save_tiff(src, "out.tif");
  #endif
    
    Image<RGBAF> src;
    
    //if(!load_tif(src, "data/tmp/P1220980-Panorama-L.tif"))
    if(!load_tif(src, "data/tmp/egypt-small.tif"))
    {
        fprintf(stderr, "Failed to load input image\n");
        return EXIT_FAILURE;
    }
    
    double_rotate_test(src);
    
    
    return 0;
}

