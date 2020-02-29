#include <cmath>
#include <cstdio>
#include <random>
using namespace std;

struct Vec3
{
    double x;
    double y;
    double z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double X, double Y, double Z) : x(X), y(Y), z(Z) {}
};

struct LatLong
{
    double lat;
    double long_;
    
    LatLong() : lat(0), long_(0) {}
    LatLong(double LAT, double LONG) : lat(LAT), long_(LONG) {}
};

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

double deg2rad(double deg)
{
    return M_PI / 180.0 * deg;
}

double rad2deg(double rad)
{
    return 180.0 / M_PI * rad;
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


int main()
{
    std::mt19937 rng;
    std::uniform_real_distribution<double> random_turn(0, 2*M_PI);
    std::uniform_real_distribution<double> random_vertical(-M_PI/2, M_PI/2);

    FILE* fp = fopen("./tmp/list.txt", "rb");
    
    //for(size_t i = 0; i < 1024*1024*1024; i++)
    while(!feof(fp))
    {
        double lat = 0, lon = 0;
        if(fscanf(fp, "%lf %lf\n", &lat, &lon) != 2)
        {
            printf("WTF!\n");
            exit(1);
        }
        
        lon = fmod(lon, 2*M_PI);
        
        //LatLong LL(random_vertical(rng), random_turn(rng));
        LatLong LL(lat, lon);
        Vec3 v = latlong_to_vec3(LL);
        LatLong LL2 = vec3_to_latlong(v);
        
        if(LL != LL2)
        {
            printf("BAD: %f %f != %f %f\n", LL.lat, LL.long_, LL2.lat, LL2.long_);
            break;
        }
    }
    
    return 0;
}

