#include "math.h"
#include "image.h"
#include "remap.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
using namespace std;

const char* USAGE =

"panorotate\n"
"\n"
"FIXME: Usage message\n"
;


void double_rotate_test(const Image<RGBAF>& src);


// panorotate -i file -o file x y z
// panorotate -i file -test

int main(int argc, char** argv)
{
    string input_filename;
    string output_filename;
    bool run_test = false;  // true if double rotate test is requested

    double rargs[3];        // rotation arguments as array
    int ri = 0;             // current parse position for rotation args
    
    // just to make it clear which is which:
    double& rx = rargs[0];
    double& ry = rargs[1];
    double& rz = rargs[2];
    
    
    if(argc <= 1)
    {
        puts(USAGE);
        return 0;
    }
    
    for(int i = 1; i < argc; i++)
    {
        string arg = argv[i];
                
        if(arg == "-h" || arg == "--help")
        {
            puts(USAGE);
            return 0;
        }
        
        if(arg == "--test")
        {
            run_test = true;
            continue;
        }
        
        if(arg == "-i")
        {
            i++;
            
            if(i >= argc)
            {
                fprintf(stderr, "Expected input filename in arguments\n");
                return EXIT_FAILURE;
            }
            
            input_filename = argv[i];
            continue;
        }
        
        if(arg == "-o")
        {
            i++;
            
            if(i >= argc)
            {
                fprintf(stderr, "Expected output filename in arguments\n");
                return EXIT_FAILURE;
            }
            
            output_filename = argv[i];
            continue;
        }
        
        if(ri >= 3)
        {
            fprintf(stderr, "Only three rotation arguments are allowed\n");
            return EXIT_FAILURE;
        }
        
        if(sscanf(argv[i], "%lf", &rargs[ri]))
        {
            rargs[ri] = deg2rad(rargs[ri]);
            ri++;
        }
    }
    
    Image<RGBAF> src, dst;
    
    if(input_filename.size() == 0)
    {
        fprintf(stderr, "[ERROR] Cannot proceed without input file\n");
        return EXIT_FAILURE;
    }
    
    if(!run_test && output_filename.size() == 0)
    {
        fprintf(stderr, "[ERROR] No output filename specified\n");
        return EXIT_FAILURE;
    }
    
    if(!load(src, input_filename))
    {
        fprintf(stderr, "[ERROR] Couldn't load input file -- stopping.\n");
        return EXIT_FAILURE;
    }
    
    if(run_test)
    {
        double_rotate_test(src);
        return 0;
    }
    
    dst.resize(src.width, src.height);
    
    printf("Input:    %s\n", input_filename.c_str());
    printf("Output:   %s\n", output_filename.c_str());
    printf("Size:     %lu %lu\n", src.width, src.height);
    printf("Rotation: %f %f %f\n", rad2deg(rx), rad2deg(ry), rad2deg(rz));

    
    remap_full3(dst, src, rotZ(rz)*rotY(ry)*rotX(rx));
    save_tiff(dst, output_filename);
    
    return 0;
}

