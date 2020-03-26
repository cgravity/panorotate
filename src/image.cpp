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
ImageLoadResult load_tiff(Image<RGBAF>& into, const std::string& path)
{
    ImageLoadResult result;
    
    TIFF* tif = TIFFOpen(path.c_str(), "r");
    
    if(!tif)
        return result;
    
    uint32 width;
    uint32 height;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    
    uint32 bps = 0;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    uint32 spp = 0;
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
    
    result.bps = bps;
    result.spp = spp;
    
    double scale;
    int bytes;
    
    if(bps == 8)
    {
        scale = 0xFF;
        bytes = 1;
    }
    else if(bps == 16)
    {
        scale = 0xFFFF;
        bytes = 2;
    }
    else
    {
        fprintf(stderr, "[ERROR] TIFF with unsupported bits per sample: %u\n",
            bps);
        
        TIFFClose(tif);
        return result;
    }
    
    if(spp != 3 && spp != 4)
    {
        fprintf(stderr, "[ERROR] TIFF with unsupported samples per pixel: %u\n",
            spp);
        
        TIFFClose(tif);
        return result;
    }
    
    into.resize(width, height);
    
    unsigned char* buffer = (unsigned char*)malloc(width*spp*bytes);
    
    for(uint32_t row = 0; row < height; row++)
    {
        TIFFReadScanline(tif, buffer, row);
        for(uint32 col = 0; col < width; col++)
        {
            RGBAF color;
            
            if(bps == 8)
            {
                unsigned char* pixel = &buffer[spp*col];

                color.r = *(pixel + 0) / scale;
                color.g = *(pixel + 1) / scale;
                color.b = *(pixel + 2) / scale;
                
                if(spp == 4)
                {
                    color.a = *(pixel + 3) / scale;
                }
                else
                {
                    color.a = 1.0;
                }
            }
            else
            {
                uint16_t* pixel = &((uint16_t*)buffer)[spp*col];

                color.r = *(pixel + 0) / scale;
                color.g = *(pixel + 1) / scale;
                color.b = *(pixel + 2) / scale;
                
                if(spp == 4)
                {
                    color.a = *(pixel + 3) / scale;
                }
                else
                {
                    color.a = 1.0;
                }
            }
            
            into.put(col, row, color);
        }
    }
    
    free(buffer);
    TIFFClose(tif);
    
    result.ok = true;
    return result;
}

ImageLoadResult load_jpeg(Image<RGBAF>& into, const std::string& path)
{
    ImageLoadResult result;     // ok = false by default
    
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    
    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));
    cinfo.err = jpeg_std_error(&jerr);
    
    jpeg_create_decompress(&cinfo);
    
    FILE* fp = fopen(path.c_str(), "rb");
    if(!fp)
    {
        fprintf(stderr, "[ERROR] Failed to open: %s\n", path.c_str());
        jpeg_destroy_decompress(&cinfo);
        return result;
    }
    
    jpeg_stdio_src(&cinfo, fp);
    
    if(jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
    {
        fprintf(stderr, "[ERROR] Failed to read JPG header: %s\n",path.c_str());
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
        return result;
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
    
    result.ok = true;
    return result;
}

void save_jpeg(const Image<RGBAF>& from, const std::string& path, 
    ImageSaveParams params)
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
    jpeg_set_quality(&cinfo, params.quality, TRUE);
    
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

void save_tiff(const Image<RGBAF>& from, const std::string& path, 
    ImageSaveParams params)
{
    unsigned char* row_bytes = NULL;
    uint16_t* row_shorts = NULL;

    if(params.spp !=3 && params.spp != 4)
    {
        fprintf(stderr, "[ERROR] Unsupported output TIFF SPP: %d\n",
            params.spp);
        
        return;
    }

    if(params.bps == 8)
    {
        row_bytes = (unsigned char*)malloc(from.width * params.spp);
        memset(row_bytes, '\0', from.width * params.spp);
    }
    else if(params.bps == 16)
    {
        row_shorts = (uint16_t*)malloc(from.width * params.spp * 2);
        memset(row_shorts, '\0', from.width * params.spp * 2);
    }
    else
    {
        fprintf(stderr, "[ERROR] Unsupported output TIFF BPS: %d\n", 
            params.bps);
        return;
    }
    
    
    TIFF* tif = TIFFOpen(path.c_str(), "w");
    
    if(!tif)
    {
        perror("save_tiff");
        return;
    }
    
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, from.width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, from.height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, params.spp);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, params.bps);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);    
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    
    if(params.spp == 4)
    {
        uint16 extra_list[1] = {EXTRASAMPLE_ASSOCALPHA};
        TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &extra_list);
    }
        
    
    for(size_t y = 0; y < from.height; y++)
    {
        for(size_t x = 0; x < from.width; x++)
        {
        
            if(params.bps == 8)
            {
                row_bytes[params.spp*x+0] = 0xFF * from.get(x,y).r;
                row_bytes[params.spp*x+1] = 0xFF * from.get(x,y).g;
                row_bytes[params.spp*x+2] = 0xFF * from.get(x,y).b;
                
                if(params.spp == 4)
                {
                    row_bytes[params.spp*x+3] = 0xFF * from.get(x,y).a;
                }
                
                
                TIFFWriteScanline(tif, row_bytes, y, 0);
            }
            else if(params.bps == 16)
            {
                row_shorts[params.spp*x+0] = 0xFFFF * from.get(x,y).r;
                row_shorts[params.spp*x+1] = 0xFFFF * from.get(x,y).g;
                row_shorts[params.spp*x+2] = 0xFFFF * from.get(x,y).b;
                
                if(params.spp == 4)
                {
                    row_shorts[params.spp*x+3] = 0xFFFF * from.get(x,y).a;
                }
                        
                TIFFWriteScanline(tif, row_shorts, y, 0);
            }
            else
            {
                fprintf(stderr, "[ERROR] save_tiff invalid state\n");
                TIFFClose(tif);
                return;
            }
        }
    }
    
    TIFFClose(tif);
}

void convert_image(Image<RGB8>& dst, const Image<RGBAF>& src)
{
    dst.resize(src.width, src.height);
    
    for(size_t y = 0; y < src.height; y++)
    for(size_t x = 0; x < src.width; x++)
    {
        RGB8 pixel;
        pixel.r = 0xFF * src.get(x,y).r;
        pixel.g = 0xFF * src.get(x,y).g;
        pixel.b = 0xFF * src.get(x,y).b;
        dst.put(x,y,pixel);
    }
}

ImageLoadResult load(Image<RGBAF>& into, const std::string& path)
{
    ImageLoadResult bad_result;     // ok = false by default
    
    char buffer[16];
    memset(buffer, '\0', sizeof(buffer));
    
    FILE* fp = fopen(path.c_str(), "rb");
    
    if(!fp)
    {
        fprintf(stderr, "[ERROR] Failed to open file: %s\n", path.c_str());
        return bad_result;
    }
    
    size_t read_size = fread(buffer, 1, 16, fp);
    fclose(fp);
    
    if(read_size != 16)
    {
        fprintf(stderr, "[ERROR] Failed to read from file: %s\n", path.c_str());
        return bad_result;
    }
    
    // PNG
    if(memcmp(buffer, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0)
    {
        fprintf(stderr, "[ERROR] PNG input type is not supported yet\n");
        return bad_result;
    }
    
    // JPG
    if(memcmp(buffer, "\xFF\xD8\xFF\xE0", 4) == 0 ||
       memcmp(buffer, "\xFF\xD8\xFF\xE1", 4) == 0)
    {
        return load_jpeg(into, path);
    }
    
    // TIFF
    if( memcmp(buffer, "\x49\x49\x2A\x00", 4) == 0 || 
        memcmp(buffer, "\x4D\x4D\x00\x2A", 4) == 0)
    {
        return load_tiff(into, path);
    }
    
    fprintf(stderr, "[ERROR] Did not recognize input file format.\n");
    return bad_result;
}

