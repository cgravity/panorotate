#pragma once

#include <vector>
#include <string>

struct RGB8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    RGB8() : r(0), g(0), b(0) {}
};

/*
struct RGBA8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    
    RGBA8() : r(0), g(0), b(0), a(0) {}
};

struct RGB16
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    
    RGB16() : r(0), g(0), b(0) {}
};

struct RGBA16
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
    
    RGBA16() : r(0), g(0), b(0), a(0) {}
};
*/

struct RGBAF
{
    double r;
    double g;
    double b;
    double a;
    
    RGBAF() : r(0), g(0), b(0), a(0) {}
    RGBAF(double R, double G, double B) : r(R), g(G), b(B), a(1) {}
    RGBAF(double R, double G, double B, double A) : r(R), g(G), b(B), a(A) {}
};

bool exact_match(const RGBAF& a, const RGBAF& b);

RGBAF operator+(const RGBAF& c0, const RGBAF& c1);
RGBAF operator*(double s, const RGBAF& c);
RGBAF& operator+=(RGBAF& onto, const RGBAF& from);
RGBAF& operator*=(RGBAF& onto, double s);

/*
 *  AB  A = (0, 0); B = (1, 0)
 *  CD  C = (0, 1); D = (1, 1)
 *
 *  0.0 <= x <= 1.0
 *  0.0 <= y <= 1.0
 */
double bilinear(double x, double y, double* values);
RGBAF bilinear(double x, double y, RGBAF* values);

template<typename T>
struct Image
{
    std::vector<T> values;
    size_t width;
    size_t height;
    
    Image() : width(0), height(0) {}
    Image(size_t w, size_t h) : width(w), height(h)
    {
        values.resize(w*h);
    }
    
    void clear(const T& value)
    {
        for(size_t y = 0; y < height; y++)
        for(size_t x = 0; x < width; x++)
            values[width*y + x] = value;
    }
    
    void clear()
    {
        clear(T());
    }
    
    void resize(size_t W, size_t H)
    {
        width = W;
        height = H;
        values.resize(W*H);
    }
    
    void put(size_t x, size_t y, const T& value)
    {
        values.at(width * y + x) = value;
    }
    
    T& get(size_t x, size_t y)
    {
        return values.at(width * y + x);
    }
    
    const T& get(size_t x, size_t y) const
    {
        return values.at(width * y + x);
    }
    
    T get_clamp(int x, int y) const
    {
        if(x < 0)
            x = 0;
        else if(x >= width)
            x = width-1;

        if(y < 0)
            y = 0;
        else if(y >= height)
            y = height-1;
        
        return get(x, y);        
    } 
};

RGBAF bilinear_get(const Image<RGBAF>& src, double x, double y);

struct ImageLoadResult
{
    bool ok;
    
    uint32_t bps;
    uint32_t spp;
    
    ImageLoadResult() : ok(false), bps(8), spp(3) {}
};

ImageLoadResult load(Image<RGBAF>& into, const std::string& path);
ImageLoadResult load_tiff(Image<RGBAF>& into, const std::string& path);
ImageLoadResult load_jpeg(Image<RGBAF>& into, const std::string& path);


struct ImageSaveParams
{
    // save_tiff needs this info:
    uint32_t bps;
    uint32_t spp;
    
    // save_jpeg needs this info:
    int quality;
    
    ImageSaveParams() : bps(8), spp(3), quality(90) {}
};

void save_jpeg(const Image<RGBAF>& from, const std::string& path,
    ImageSaveParams params);
    
void save_tiff(const Image<RGBAF>& from, const std::string& path, 
    ImageSaveParams params);


void convert_image(Image<RGB8>& dst, const Image<RGBAF>& src);



