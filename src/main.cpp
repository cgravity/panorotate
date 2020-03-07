#include <cmath>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <cstdlib>
using namespace std;

#include "jpeglib.h"
#include "tiffio.h"

double deg2rad(double deg)
{
    return M_PI / 180.0 * deg;
}

double rad2deg(double rad)
{
    return 180.0 / M_PI * rad;
}

struct Vec3
{
    double x;
    double y;
    double z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double X, double Y, double Z) : x(X), y(Y), z(Z) {}
};

struct Mat3
{
    double value[9];
    
    Mat3() : value{0,0,0,0,0,0,0,0,0} {}
    
    void reset()
    {
        for(int i = 0; i < 9; i++)
            value[i] = 0;
    }
    
    const double& operator[](int i) const
    {
        return value[i];
    }
    
    double& operator[](int i)
    {
        return value[i];
    }
};

Vec3 operator*(const Mat3& m, const Vec3& v)
{
    Vec3 out;
    out.x = v.x * m[0] + v.y * m[1] + v.z * m[2];
    out.y = v.x * m[3] + v.y * m[4] + v.z * m[5];
    out.z = v.x * m[6] + v.y * m[7] + v.z * m[8];
    return out;
}

Mat3 operator*(const Mat3& m1, const Mat3& m2)
{
    Mat3 out;
    
    for(int r = 0; r < 3; r++)
    for(int c = 0; c < 3; c++)
    {
        for(int i = 0; i < 3; i++)
        {
            out[3*r + c] += m1[3*r+i] * m2[3*i+c];
        }
    }
    
    return out;
}

struct LatLong
{
    double lat;
    double long_;
    
    LatLong() : lat(0), long_(0) {}
    LatLong(double LAT, double LONG) : lat(LAT), long_(LONG) {}
};

bool operator==(const LatLong& a, const LatLong& b)
{
    const double EPS = 0.000001;
    return fabs(a.lat - b.lat) < EPS && fabs(a.long_ - b.long_) < EPS;
}

bool operator!=(const LatLong& a, const LatLong& b)
{
    return !(a == b);
}

Vec3 latlong_to_vec3(const LatLong& LL)
{
    Vec3 result;
    
    result.x = cos(LL.long_) * cos(LL.lat);
    result.y = sin(LL.long_) * cos(LL.lat);
    result.z = sin(LL.lat);
    
    return result;
}

LatLong vec3_to_latlong(const Vec3& v)
{
    LatLong result;
    result.long_ = atan2(v.y, v.x);
    while(result.long_ < 0)
        result.long_ += 2*M_PI;
    
    result.lat = asin(v.z);
    
    return result;
}

// rotates Y towards Z (by spinning X axis)
Mat3 rotX(double r)
{
    Mat3 m;
    m[0] = 1.0;
    m[4] = cos(r);
    m[7] = sin(r);
    m[5] = -sin(r);
    m[8] = cos(r);
    
    return m;
}

// rotates X towards Z (by spinning Y axis)
Mat3 rotY(double r)
{
    Mat3 m;
    m[4] = 1.0;
    m[0] = cos(r);
    m[6] = sin(r);
    m[2] = -sin(r);
    m[8] = cos(r);
    return m;
}

// rotates X towards Y (by spinning Z axis)
Mat3 rotZ(double r)
{
    Mat3 m;
    m[8] = 1.0;
    m[0] = cos(r);
    m[3] = sin(r);
    m[1] = -sin(r);
    m[4] = cos(r);
    return m;
}

/*
struct RGB8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    RGB8() : r(0), g(0), b(0) {}
};

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
//template<typename ValueType>
//ValueType bilinear(double x, double y, ValueType* values)
static inline double bilinear(double x, double y, double* values)
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

template<typename T>
struct Image
{
    vector<T> values;
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

static inline RGBAF bilinear_get(const Image<RGBAF>& src, double x, double y)
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


Mat3 ident()
{
    Mat3 m;
    m[0] = 1;
    m[4] = 1;
    m[8] = 1;
    return m;
}

void remap_full(Image<RGBAF>& onto, const Image<RGBAF>& from, Mat3 rot = ident())
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
            double src_y = (M_PI - (LL_src.lat+(M_PI/2)))/ M_PI * (from.height-1);
            
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

void remap_full2(Image<RGBAF>& onto, const Image<RGBAF>& from, Mat3 rot = ident())
{
    // these should be odd to ensure we hit the center of AA range exactly
    const int XSAMPS = 9;
    const int YSAMPS = 9;
    
    double filter_table[XSAMPS*YSAMPS];
    const double s = 0.4; // sigma for gaussian -- FIXME: validate if good value
    
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
            double yf = (double)y + 1.0/(YSAMPS-1) * sub_y - 0.5;
            double xf = (double)x + 1.0/(XSAMPS-1) * sub_x - 0.5;
                
            LatLong LL(
                M_PI/2 - (double)yf / (onto.height-1) * M_PI,
                (double)xf/(onto.width-1) * 2*M_PI);
                
            
            Vec3 v = latlong_to_vec3(LL);
            v = rot * v;
            LatLong LL_src = vec3_to_latlong(v);
            
            double src_x = LL_src.long_ / (2*M_PI) * (from.width-1);
            double src_y = (M_PI - (LL_src.lat+(M_PI/2)))/ M_PI * (from.height-1);
            
            if(src_x > from.width-1)
                src_x = from.width - 1;
            if(src_y > from.height-1)
                src_y = from.height - 1;
            if(src_x < 0)
                src_x = 0;
            if(src_y < 0)
                src_y = 0;
            
            //out_pixel += bilinear_get(from, src_x, src_y);
            
            double scale = filter_table[sub_y*XSAMPS + sub_x];
            out_pixel += scale * bilinear_get(from, src_x, src_y);
        }
        
        //out_pixel *= 1.0 / (XSAMPS*YSAMPS);
        
        onto.put(x,y,out_pixel);
    }
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

bool load(Image<RGBAF>& into, const std::string& path)
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

void save_blob(const Image<RGBAF>& src, const std::string& path)
{
    FILE* fp = fopen(path.c_str(), "wb");
    
    for(size_t y = 0; y < src.height; y++)
    {
        for(size_t x = 0; x < src.width; x++)
        {
            unsigned char r = 0xFF * src.get(x,y).r;
            unsigned char g = 0xFF * src.get(x,y).g;
            unsigned char b = 0xFF * src.get(x,y).b;
            
            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }
    
    fclose(fp);
}

void make_test_image(Image<RGBAF>& into)
{
    into.clear(RGBAF(0,0,0,0));
    
    for(int y = 0; y < into.height; y++)
    for(int x = 0; x < into.width; x++)
    {
        RGBAF pixel;
        pixel.r = (float)x / into.width;
        pixel.g = (float)y / into.height;
        pixel.b = 
          sqrt(x*x + y*y) / sqrt(into.width*into.width+into.height*into.height);
        
        into.put(x,y,pixel);  
    }
}

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
    //Image<RGBAF> src, dst(4096*2, 4096);
    Image<RGBAF> src, dst;
    //if(!load(src, "data/constellations_2048.jpg"))
    //if(!load(src, "data/WI-Capitol-360x180-L.jpg"))

    
    dst.resize(src.width, src.height);
    remap_full(dst, src, rotZ(rz)*rotY(ry)*rotX(rx));
  #endif
    
    Image<RGBAF> src, dst;
    
    //if(!load_tif(src, "data/tmp/P1220980-Panorama-L.tif"))
    if(!load_tif(src, "data/tmp/egypt-small.tif"))
    {
        fprintf(stderr, "Failed to load input image\n");
        return EXIT_FAILURE;
    }
    //src.resize(1920,1920/2);
    //make_test_image(src);
    
    save_tiff(src, "tmp/test.tif");
    
    dst.resize(src.width, src.height);
    
    remap_full(dst, src, rotX(deg2rad(90)));
    save_tiff(dst, "tmp/test2.tif");
    
    remap_full2(dst, src, rotX(deg2rad(90)));
    save_tiff(dst, "tmp/test3.tif");
    
    #if 0
    for(size_t y = 0; y < dst.height; y++)
    for(size_t x = 0; x < dst.width; x++)
    {
        RGBAF a = src.get(x,y);
        RGBAF b = dst.get(x,y);
        RGBAF c(b.r - a.r, 
                b.g - a.g, 
                b.b - a.b, 
                b.a - a.a);
        
        if(c.r != 0 || c.g != 0 || c.b != 0)
        {
            dst.put(x,y,b);
        }
        else
            dst.put(x,y,RGBAF(0,0,0,0));
    }
    
    save_blob(src, "tmp/blob1");
    save_blob(dst, "tmp/blob2");
    #endif
    
    return 0;
}

