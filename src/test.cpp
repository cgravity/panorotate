#include <cstdio>
#include <cmath>
using namespace std;

#include "custom_math.h"
#include "image.h"
#include "remap.h"
#include "test.h"

// rotate 90 degrees, rotate back, calculate and print stats
void double_rotate_test(const Image<RGBAF>& src)
{
    Image<RGBAF> dst, dst2;
    
    dst.resize(src.width, src.height);
    dst2.resize(src.width, src.height);
    
    printf("Rotating...\n");
    //remap_full1(dst, src, rotX(deg2rad(90)));
    remap_full3(dst, src, rotX(deg2rad(90)), 0.001);
    
    printf("Rotating back...\n");
    //remap_full1(dst2, dst, rotX(deg2rad(-90)));
    remap_full3(dst2, dst, rotX(deg2rad(-90)),  0.001);
    
    printf("Converting to 8-bit...\n");
    Image<RGB8> out_src, out_dst;
    
    convert_image(out_src, src);
    convert_image(out_dst, dst2);
    
    printf("Computing stats...\n");
    printf("\n");

    RGBAF sad(0,0,0,0);
    RGBAF ssd(0,0,0,0);
    
    RGBAF sad8(0,0,0,0);
    RGBAF ssd8(0,0,0,0);
    
    for(size_t y = 0; y < src.height; y++)
    for(size_t x = 0; x < src.width; x++)
    {
        RGBAF a = src.get(x,y);
        RGBAF b = dst2.get(x,y);
        
        
        RGB8 a8 = out_src.get(x,y);
        RGB8 b8 = out_dst.get(x,y);
        
        double R = a.r - b.r;
        double G = a.g - b.g;
        double B = a.b - b.b;
        
        double R8 = a8.r - b8.r;
        double G8 = a8.g - b8.g;
        double B8 = a8.b - b8.b;
        
        sad.r += fabs(R);
        sad.g += fabs(G);
        sad.b += fabs(B);
        
        sad8.r += fabs(R8);
        sad8.g += fabs(G8);
        sad8.b += fabs(B8);
                
        ssd.r += R*R;
        ssd.g += G*G;
        ssd.b += B*B;
        
        ssd8.r += R8*R8;
        ssd8.g += G8*G8;
        ssd8.b += B8*B8;
    }
    
    RGBAF mad = sad;
    mad.r /= src.width; mad.r /= src.height;
    mad.g /= src.width; mad.g /= src.height;
    mad.b /= src.width; mad.b /= src.height;
    
    RGBAF mad8 = sad8;
    mad8.r /= src.width; mad8.r /= src.height;
    mad8.g /= src.width; mad8.g /= src.height;
    mad8.b /= src.width; mad8.b /= src.height;
    
    
    printf("Sum of absolute differences (float):\n"
           "Red:\t%f\n"
           "Green:\t%f\n"
           "Blue:\t%f\n",
           sad.r, sad.g, sad.b);
    printf("\n");
    
    printf("Mean absolute differences (float):\n"
           "Red:\t%f\n"
           "Green:\t%f\n"
           "Blue:\t%f\n",
           mad.r, mad.g, mad.b);
    printf("\n");
    
    printf("Sum of squared differences (float):\n"
           "Red:\t%f\n"
           "Green:\t%f\n"
           "Blue:\t%f\n",
           ssd.r, ssd.g, ssd.b);
    
    printf("\n");
    printf("\n");    
    
    printf("Sum of absolute differences (RGB8):\n"
           "Red:\t%f\n"
           "Green:\t%f\n"
           "Blue:\t%f\n",
           sad8.r, sad8.g, sad8.b);
    printf("\n");
    
    printf("Mean absolute differences (RGB8):\n"
           "Red:\t%f\n"
           "Green:\t%f\n"
           "Blue:\t%f\n",
           mad8.r, mad8.g, mad8.b);
    printf("\n");
    
    printf("Sum of squared differences (RGB8):\n"
           "Red:\t%f\n"
           "Green:\t%f\n"
           "Blue:\t%f\n",
           ssd8.r, ssd8.g, ssd8.b);
}

