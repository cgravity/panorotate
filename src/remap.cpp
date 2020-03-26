#include "custom_math.h"
#include "image.h"
#include "remap.h"

#include <cmath>
using namespace std;


void remap_full3(Image<RGBAF>& onto, const Image<RGBAF>& from, Mat3 rot,
    const double s)
{
    LL2Vec3_Table lookup_table(onto.width, onto.height, 9);
    
    // these should be odd to ensure we hit the center of AA range exactly
    const int XSAMPS = 9;
    const int YSAMPS = 9;
    
    double filter_table[XSAMPS*YSAMPS];
    
    for(int y = 0; y < YSAMPS; y++)
    for(int x = 0; x < XSAMPS; x++)
    {
        double X = x - (double)XSAMPS/2.0;
        double Y = y - (double)YSAMPS/2.0;
        
        filter_table[XSAMPS*y+x] = exp(-(X*X + Y*Y)/(2*s));
    }
    
    double sum = 0.0;
    for(int i = 0; i < XSAMPS*YSAMPS; i++)
    {
        sum += filter_table[i];
    }
    
    for(int i = 0; i < XSAMPS*YSAMPS; i++)
    {
        filter_table[i] /= sum;
    }

    #pragma omp parallel for
    for(size_t y = 0; y < onto.height; y++)
    for(size_t x = 0; x < onto.width; x++)
    {
        RGBAF out_pixel;
        
        for(int sub_y = 0; sub_y < YSAMPS; sub_y++)
        for(int sub_x = 0; sub_x < XSAMPS; sub_x++)
        {
            Vec3 v = lookup_table.lookup(x, sub_x, y, sub_y);
            v = rot * v;
            LatLong LL_src = vec3_to_latlong(v);
            
            double src_x = LL_src.long_ / (2*M_PI) * (from.width-1);
            double src_y = 
                (M_PI - (LL_src.lat+(M_PI/2)))/ M_PI * (from.height-1);
            
            if(src_x > from.width-1)
                src_x = from.width - 1;
            if(src_y > from.height-1)
                src_y = from.height - 1;
            if(src_x < 0)
                src_x = 0;
            if(src_y < 0)
                src_y = 0;
            
            double scale = filter_table[sub_y*XSAMPS + sub_x];
            out_pixel += scale * bilinear_get(from, src_x, src_y);
        }
        
        onto.put(x,y,out_pixel);
    }
}


// obsolete -- only included still for quality comparison
void remap_full1(Image<RGBAF>& onto, const Image<RGBAF>& from, Mat3 rot)
{
    #pragma omp parallel for
    for(size_t y = 0; y < onto.height; y++)
    for(size_t x = 0; x < onto.width; x++)
    {
        RGBAF out_pixel;
    
        for(int sub_y = 0; sub_y <= 5; sub_y++)
        for(int sub_x = 0; sub_x <= 5; sub_x++)
        {
            double yf = (double)y + 1.0/5.0 * sub_y;
            double xf = (double)x + 1.0/5.0 * sub_x;
            
            LatLong LL(
                M_PI/2 - (double)yf / (onto.height-1) * M_PI,
                (double)xf/(onto.width-1) * 2*M_PI);
            
            Vec3 v = latlong_to_vec3(LL);
            v = rot * v;
            LatLong LL_src = vec3_to_latlong(v);
            
            double src_x = LL_src.long_ / (2*M_PI) * (from.width-1);
            double src_y = 
                (M_PI - (LL_src.lat+(M_PI/2)))/ M_PI * (from.height-1);
            
            if(src_x > from.width-1)
                src_x = from.width - 1;
            if(src_y > from.height-1)
                src_y = from.height - 1;
            
            out_pixel += bilinear_get(from, src_x, src_y);
        }
        
        out_pixel.r /= 36.0;
        out_pixel.g /= 36.0;
        out_pixel.b /= 36.0;
        out_pixel.a /= 36.0;
            
        onto.put(x,y,out_pixel);
    }
}


