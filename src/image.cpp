#include "image.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace std;

#include "jpeglib.h"
#include "tiffio.h"


bool exact_match(const RGBAF& a, const RGBAF& b)
{
    if(a.r != b.r ||
       a.g != b.g ||
       a.b != b.b ||
       a.a != b.a)
    {
        return false;
    }
    
    return true;
}

RGBAF operator+(const RGBAF& c0, const RGBAF& c1)
{
    RGBAF out;
    out.r = c0.r + c1.r;
    out.g = c0.g + c1.g;
    out.b = c0.b + c1.b;
    out.a = c0.a + c1.a;
    return out;
}

RGBAF& operator+=(RGBAF& onto, const RGBAF& from)
{
    onto = onto + from;
    return onto;
}

RGBAF operator*(double s, const RGBAF& c)
{
    RGBAF out;
    out.r = s * c.r;
    out.g = s * c.g;
    out.b = s * c.b;
    out.a = s * c.a;
    return out;
}

RGBAF& operator*=(RGBAF& onto, double s)
{
    onto = s * onto;
    return onto;
}

/*
 *  AB  A = (0, 0); B = (1, 0)
 *  CD  C = (0, 1); D = (1, 1)
 *
 *  0.0 <= x <= 1.0
 *  0.0 <= y <= 1.0
 */
double bilinear(double x, double y, double* values)
{
    double A = values[0];
    double B = values[1];
    double C = values[2];
    double D = values[3];
    
    double x0 = (1-x)*A + x*B;
    double x1 = (1-x)*C + x*D;
    
    double result = (1-y)*x0 + y*x1;
    return result;
}

RGBAF bilinear(double x, double y, RGBAF* values)
{
    double r[] = {values[0].r,values[1].r,values[2].r,values[3].r};
    double g[] = {values[0].g,values[1].g,values[2].g,values[3].g};
    double b[] = {values[0].b,values[1].b,values[2].b,values[3].b};
    double a[] = {values[0].a,values[1].a,values[2].a,values[3].a};
    
    RGBAF result;
    result.r = bilinear(x, y, r);
    result.g = bilinear(x, y, g);
    result.b = bilinear(x, y, b);
    result.a = bilinear(x, y, a);
    
    return result;
}

RGBAF bilinear_get(const Image<RGBAF>& src, double x, double y)
{
    double intpart = 0.0;
    
    double x_frac = modf(x, &intpart);
    double y_frac = modf(y, &intpart);
    
    RGBAF A = src.get_clamp(floor(x), floor(y));
    RGBAF B = src.get_clamp(ceil(x), floor(y));
    RGBAF C = src.get_clamp(floor(x), ceil(y));
    RGBAF D = src.get_clamp(ceil(x), ceil(y));
    RGBAF values[] = {A,B,C,D};
    
    return bilinear(x_frac, y_frac, values);
}





// FIXME: This assumes 8-bit RGB stored as scanlines in a single page.
// TIFF allows for a hell of a lot of other possibilities, some of which
// should definitely be handled (e.g. 16-bit, as well as RGBA).
bool load_tif(Image<RGBAF>& into, const std::string& path)
{
    TIFF* tif = TIFFOpen(path.c_str(), "r");
    
    if(!tif)
        return false;
    
    uint32 width;
    uint32 height;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    
    into.resize(width, height);
    
    unsigned char* buffer = (unsigned char*)malloc(width*3);
    
    for(uint32_t row = 0; row < height; row++)
    {
        TIFFReadScanline(tif, buffer, row);
        for(uint32 col = 0; col < width; col++)
        {
            unsigned char* pixel = &buffer[3*col];
            RGBAF color;
            color.r = *(pixel + 0) / (double)0xFF;
            color.g = *(pixel + 1) / (double)0xFF;
            color.b = *(pixel + 2) / (double)0xFF;
            color.a = 1.0;
            
            into.put(col, row, color);
        }
    }
    
    free(buffer);
    TIFFClose(tif);
    return true;
}

bool load_jpeg(Image<RGBAF>& into, const std::string& path)
{
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    
    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));
    cinfo.err = jpeg_std_error(&jerr);
    
    jpeg_create_decompress(&cinfo);
    
    FILE* fp = fopen(path.c_str(), "rb");
    if(!fp)
    {
        fprintf(stderr, "Failed to open: %s\n", path.c_str());
        jpeg_destroy_decompress(&cinfo);
        return false;
    }
    
    jpeg_stdio_src(&cinfo, fp);
    
    if(jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
    {
        fprintf(stderr, "Failed to read JPG header: %s\n", path.c_str());
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return false;
    }
    
    cinfo.out_color_space = JCS_RGB;
    
    unsigned char* data = (unsigned char*)malloc(3*cinfo.image_width);
    
    into.resize(cinfo.image_width, cinfo.image_height);
    
    jpeg_start_decompress(&cinfo);
    
    size_t y = 0;
    while(cinfo.output_scanline < cinfo.image_height)
    {
        memset(data, '\0', 3*cinfo.image_width);
        jpeg_read_scanlines(&cinfo, &data, 1);
        for(size_t i = 0; i < cinfo.image_width; i++)
        {
            unsigned char* p = &data[3*i];
            
            RGBAF pixel;
            pixel.r = *(p+0) / (float)0xFF;
            pixel.g = *(p+1) / (float)0xFF;
            pixel.b = *(p+2) / (float)0xFF;
            pixel.a = 1.0;
            
            into.put(i, y, pixel);
        }
        y++;
    }
    
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    
    free(data);
    return true;
}

void save_jpeg(const Image<RGBAF>& from, const std::string& path)
{
    jpeg_compress_struct cinfo;
    jpeg_error_mgr error;
    
    cinfo.err = jpeg_std_error(&error);
    
    jpeg_create_compress(&cinfo);
    
    FILE* fp = fopen(path.c_str(), "wb");
    if(!fp)
    {
        perror("Failed to open file for writing");
        return;
    }
    
    jpeg_stdio_dest(&cinfo, fp);
    
    cinfo.image_width  = from.width;
    cinfo.image_height = from.height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 90, TRUE);
    
    jpeg_start_compress(&cinfo, TRUE);
    
    unsigned char* data = (unsigned char*)malloc(from.width*3);
    size_t y = 0;
    while(cinfo.next_scanline < cinfo.image_height)
    {
        for(size_t x = 0; x < from.width; x++)
        {
            const RGBAF& p = from.get(x,y);
            data[3*x+0] = p.r * 255;
            data[3*x+1] = p.g * 255;
            data[3*x+2] = p.b * 255;
        }
        
        jpeg_write_scanlines(&cinfo, &data, 1);
        y++;
    }

    free(data);
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
}

void save_tiff(const Image<RGBAF>& from, const std::string& path)
{
    unsigned char* row = (unsigned char*)malloc(from.width * 3);
    memset(row, '\0', from.width*3);

    TIFF* tif = TIFFOpen(path.c_str(), "w");
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, from.width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, from.height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    
    for(size_t y = 0; y < from.height; y++)
    {
        for(size_t x = 0; x < from.width; x++)
        {
            row[3*x+0] = 0xFF * from.get(x,y).r;
            row[3*x+1] = 0xFF * from.get(x,y).g;
            row[3*x+2] = 0xFF * from.get(x,y).b;
        }
        
        TIFFWriteScanline(tif, row, y, 0);
    }
    
    TIFFClose(tif);
}

void convert_image(Image<RGB8>& dst, const Image<RGBAF>& src)
{
    dst.resize(src.width, src.height);
    
    for(int y = 0; y < src.height; y++)
    for(int x = 0; x < src.width; x++)
    {
        RGB8 pixel;
        pixel.r = 0xFF * src.get(x,y).r;
        pixel.g = 0xFF * src.get(x,y).g;
        pixel.b = 0xFF * src.get(x,y).b;
        dst.put(x,y,pixel);
    }
}


