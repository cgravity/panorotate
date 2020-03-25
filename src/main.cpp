#include "math.h"
#include "image.h"
#include "remap.h"
#include "test.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <cctype>
using namespace std;

const char* USAGE =

"Usage: panorotate -i <filename> [-o <filename>] [-f <format>]\n"
"                  [--test] [<x>] [<y>] [<z>]\n"
"\n"
"Flags and arguments:\n"
"    -i filename    specify input filename (required)\n"
"    -o filename    specify output filename (required except for --test)\n"
"    -f format      specify output format (default is 'TIFF')\n"
"                   See the table below for full list of possible formats.\n"
"    --test         Run the double rotation quality test.\n"
"    <x y z>        Rotation angles in degrees (unset values default to 0)\n"
"\n"
"Example usage:\n"
"    panorotate -i input.tif -o output.tif -f TIFF_RGBA16 90 0 0\n"
"\n"
"    This loads input.tif, rotates the X axis by 90 degrees, and saves the\n"
"    output into output.tiff in 16-bit RGBA format.\n"
;


struct SaveFormat
{
    typedef void (*SaveFunc)(const Image<RGBAF>&, const std::string&);
    
    string flag_name;
    SaveFunc save;
    string description;
    
    SaveFormat(const string& flag, SaveFunc func, const string& desc) 
        : flag_name(flag), save(func), description(desc) {}
    
    bool match(const string& input)
    {
        if(input.size() != flag_name.size())
        {
            return false;
        }
        
        for(size_t i = 0; i < flag_name.size(); i++)
        {
            char a = tolower(flag_name.at(i));
            char b = tolower(input.at(i));
            
            if(a != b)
                return false;
        }
    
        return true;
    }
};

SaveFormat save_format_table[] = {
    SaveFormat("TIFF", save_tiff,  "(default) -- same bpp/spp if input was TIFF, else RGB8"),
    SaveFormat("JPG", save_jpeg,   "8-bit RGB JPEG (90 percent quality)"),
    SaveFormat("JPEG", save_jpeg,  "(synonym; same as above)") //,
    /*
    SaveFormat{"TIFF_RGB8", save_tiff_rgb8, "8-bit RGB TIFF"),
    SaveFormat{"TIFF_RGB16", save_tiff_rgb16, "16-bit RGB TIFF"),
    SaveFormat{"TIFF_RGBA8", save_tiff_rgba8, "8-bit RGBA TIFF"),
    SaveFormat{"TIFF_RGBA16", save_tiff_rgba16, "16-bit RGBA TIFF")
    */
    
     ,   SaveFormat("TIFF_RGBA16", save_tiff, "FIXME")
};

void print_save_formats()
{
    printf("Recognized save format flags (for -f):\n");
    
    // calculate max flag_name length to make formating nicer
    size_t max_length = 0;

    for(int i = 0; 
        i < sizeof(save_format_table)/sizeof(save_format_table[0]);
        i++)
    {
        size_t len = save_format_table[i].flag_name.size();
        
        if(len > max_length)
            max_length = len;
    }
    
    // actually print the table
    for(int i = 0; 
        i < sizeof(save_format_table)/sizeof(save_format_table[0]);
        i++)
    {
        SaveFormat& format = save_format_table[i];
        printf("    ");
        printf("%s", format.flag_name.c_str());
        
        for(size_t c = format.flag_name.size() ; c < max_length + 4; c++)
        {
            putc(' ', stdout);
        }
        
        printf("%s\n", format.description.c_str());
    }
}

void print_usage()
{
    puts(USAGE);
    print_save_formats();
}

SaveFormat* find_save_format(const std::string& name)
{
    for(int i = 0; 
        i < sizeof(save_format_table)/sizeof(save_format_table[0]);
        i++)
    {
        if(save_format_table[i].match(name))
            return &save_format_table[i];
    }
    
    return (SaveFormat*)0;
}



// panorotate -i file -o file x y z
// panorotate -i file -test

int main(int argc, char** argv)
{
    SaveFormat* save_format = &save_format_table[0];
    
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
        print_usage();
        return 0;
    }
    
    for(int i = 1; i < argc; i++)
    {
        string arg = argv[i];
                
        if(arg == "-h" || arg == "--help")
        {
            print_usage();
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
        
        if(arg == "-f")
        {
            i++;
            
            if(i >= argc)
            {
                fprintf(stderr, "Expected output format after -f flag\n");
                return EXIT_FAILURE;
            }
            
            save_format = find_save_format(argv[i]);
            
            if(!save_format)
            {
                fprintf(stderr, "[ERROR] Unknown save format: %s\n", argv[i]);
                print_save_formats();
                return EXIT_FAILURE;
            }
            
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
    
    printf("Input:       %s\n", input_filename.c_str());
    printf("Output:      %s\n", output_filename.c_str());
    printf("Output type: %s\n", save_format->flag_name.c_str());
    printf("Size:        %lu %lu\n", src.width, src.height);
    printf("Rotation:    %f %f %f\n", rad2deg(rx), rad2deg(ry), rad2deg(rz));

    
    remap_full3(dst, src, rotZ(rz)*rotY(ry)*rotX(rx));
    save_format->save(dst, output_filename);
    
    return 0;
}

