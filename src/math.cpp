#include "math.h"
#include <cmath>
using namespace std;

double deg2rad(double deg)
{
    return M_PI / 180.0 * deg;
}

double rad2deg(double rad)
{
    return 180.0 / M_PI * rad;
}

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

bool operator==(const LatLong& a, const LatLong& b)
{
    const double EPS = 0.000001;
    return fabs(a.lat - b.lat) < EPS && fabs(a.long_ - b.long_) < EPS;
}

bool operator!=(const LatLong& a, const LatLong& b)
{
    return !(a == b);
}

LL2Vec3_Table::LL2Vec3_Table(int w, int h, int s) 
    : width(w), height(h), subpixels(s)
{
    for(int x = 0; x < w; x++)
    {
        for(int sub_x = 0; sub_x < subpixels; sub_x++)
        {
            double xf = (double)x + 1.0/(subpixels-1) * sub_x - 0.5;
            double long_ = (double)xf/(width-1.0) * 2*M_PI;
            
            sin_long.push_back(sin(long_));
            cos_long.push_back(cos(long_));
        }
    }
    
    for(int y = 0; y < h; y++)
    {
        for(int sub_y = 0; sub_y < subpixels; sub_y++)
        {
            double yf  = (double)y + 1.0/(subpixels-1) * sub_y - 0.5;
            double lat = M_PI/2 - (double)yf / (height-1.0) * M_PI;
            
            sin_lat.push_back(sin(lat));
            cos_lat.push_back(cos(lat));
        }
    }
}

Vec3 LL2Vec3_Table::lookup(int x, int sub_x, int y, int sub_y)
{
    int long_ = x * subpixels + sub_x;
    int lat   = y * subpixels + sub_y;
    
    Vec3 result;
    result.x = cos_long[long_] * cos_lat[lat];
    result.y = sin_long[long_] * cos_lat[lat];
    result.z = sin_lat[lat];
    
    return result;
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

Mat3 ident()
{
    Mat3 m;
    m[0] = 1;
    m[4] = 1;
    m[8] = 1;
    return m;
}


