#pragma once

#include <vector>

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

struct LatLong
{
    double lat;
    double long_;
    
    LatLong() : lat(0), long_(0) {}
    LatLong(double LAT, double LONG) : lat(LAT), long_(LONG) {}
};

struct LL2Vec3_Table
{
    std::vector<double> sin_lat;
    std::vector<double> sin_long;
    std::vector<double> cos_lat;
    std::vector<double> cos_long;
    
    int width;
    int height;
    int subpixels;
    
    LL2Vec3_Table(int w, int h, int s);
    Vec3 lookup(int x, int sub_x, int y, int sub_y);
};


double deg2rad(double deg);
double rad2deg(double rad);

Vec3 operator*(const Mat3& m, const Vec3& v);
Mat3 operator*(const Mat3& m1, const Mat3& m2);

bool operator==(const LatLong& a, const LatLong& b);
bool operator!=(const LatLong& a, const LatLong& b);

Vec3 latlong_to_vec3(const LatLong& LL);
LatLong vec3_to_latlong(const Vec3& v);


// rotates Y towards Z (by spinning X axis)
Mat3 rotX(double r);

// rotates X towards Z (by spinning Y axis)
Mat3 rotY(double r);

// rotates X towards Y (by spinning Z axis)
Mat3 rotZ(double r);

Mat3 ident();

Mat3 transpose(const Mat3&);


