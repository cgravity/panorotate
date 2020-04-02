#include "custom_math.h"
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
"                  [-q <jpg_quality>] [--test] [--preview]\n"
"                  [--order <rpy>] [<angles...>]\n"
"\n"
"Flags and arguments:\n"
"    -i filename    specify input filename (required)\n"
"    -o filename    specify output filename (required except for --test)\n"
"    -f format      specify output format (default is 'TIFF')\n"
"                   See the table below for full list of possible formats.\n"
"    -q jpg_quality Integer quality percent to use when saving as JPEG.\n"
"                   (default is 90)\n"
"    --test         Run the double rotation test and print statistics.\n"
"                   If no angles are specified by default a 90 degree\n"
"                   roll is used to test the quality.\n"
"    --preview      Perform a single sample per output pixel to create\n"
"                   a preview image more quickly.\n"
"    --order rpy    Rotation sequence to perform indicating order of\n"
"                   roll (R), pitch (P), and yaw (Y). Only 'R', 'P', 'Y'\n"
"                   may be used in the argument following --order, though\n"
"                   any number or combination of them may be provided.\n"
"                   e.g. R, RPY, RPR... if this argument is not\n"
"                   specified then the default order is RPY.\n"
"                   'RPY' means that first a roll will be performed, then\n"
"                   a pitch, and finally a yaw.\n"
"    [<angles...>]  Rotation angles in degrees.\n"
"                   Unspecified angles will be assumed to be zero.\n"
"                   Extra angles will be ignored.\n"
"\n"
"Example usage:\n"
"    panorotate -i input.tif -o output.tif -f TIFF_RGBA16 90 0 0\n"
"\n"
"    This loads input.tif, performs a 90 degree roll, and saves the\n"
"    output into output.tif in 16-bit RGBA format.\n"
;


struct SaveFormat
{
    typedef void (*SaveFunc)(
        const Image<RGBAF>&, 
        const std::string&,
        ImageSaveParams);
    
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

void save_tiff_rgb8(
    const Image<RGBAF>& img, const std::string& path, ImageSaveParams params)
{
    params.spp = 3;
    params.bps = 8;
    save_tiff(img, path, params);
}

void save_tiff_rgba8(
    const Image<RGBAF>& img, const std::string& path, ImageSaveParams params)
{
    params.spp = 4;
    params.bps = 8;
    save_tiff(img, path, params);
}

void save_tiff_rgb16(
    const Image<RGBAF>& img, const std::string& path, ImageSaveParams params)
{
    params.spp = 3;
    params.bps = 16;
    save_tiff(img, path, params);
}

void save_tiff_rgba16(
    const Image<RGBAF>& img, const std::string& path, ImageSaveParams params)
{
    params.spp = 4;
    params.bps = 16;
    save_tiff(img, path, params);
}

SaveFormat save_format_table[] = {
    SaveFormat("TIFF", save_tiff,  "(default) -- matches bps/spp from input"),
    SaveFormat("JPG", save_jpeg,   "8-bit RGB JPEG (default q=90)"),
    SaveFormat("JPEG", save_jpeg,  "(synonym; same as above)"),
    SaveFormat("TIFF_RGB8", save_tiff_rgb8,     "8-bit  RGB  TIFF"),
    SaveFormat("TIFF_RGB16", save_tiff_rgb16,   "16-bit RGB  TIFF"),
    SaveFormat("TIFF_RGBA8", save_tiff_rgba8,   "8-bit  RGBA TIFF"),
    SaveFormat("TIFF_RGBA16", save_tiff_rgba16, "16-bit RGBA TIFF")
};

void print_save_formats()
{
    printf("Recognized save format flags (for -f):\n");
    
    // calculate max flag_name length to make formating nicer
    size_t max_length = 0;

    for(size_t i = 0; 
        i < sizeof(save_format_table)/sizeof(save_format_table[0]);
        i++)
    {
        size_t len = save_format_table[i].flag_name.size();
        
        if(len > max_length)
            max_length = len;
    }
    
    // actually print the table
    for(size_t i = 0; 
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
    for(size_t i = 0; 
        i < sizeof(save_format_table)/sizeof(save_format_table[0]);
        i++)
    {
        if(save_format_table[i].match(name))
            return &save_format_table[i];
    }
    
    return (SaveFormat*)0;
}

enum RotType
{
    ROT_X,
    ROT_Y,
    ROT_Z
};

bool parse_order(vector<RotType>& out, const string& order)
{
    out.clear();
    
    for(size_t i = 0; i < order.size(); i++)
    {
        switch(order.at(i))
        {
            case 'R':   // roll
            case 'r':
                out.push_back(ROT_X);
                break;
            
            case 'P':   // pitch
            case 'p':
                out.push_back(ROT_Y);
                break;
            
            case 'Y':   // yaw
            case 'y':
                out.push_back(ROT_Z);
                break;
            
            default:
                return false;
        }
    }
    
    return true;
}

// angles specified in degrees
Mat3 make_rotation(const vector<RotType>& order, 
                   const vector<double>& angles)
{
    Mat3 accum = ident();
    
    for(size_t i = 0; i < order.size(); i++)
    {
        RotType type = order.at(i);
        double angle = deg2rad(angles.at(i));
        
        Mat3 m;
        
        switch(type)
        {
            case ROT_X:
                m = rotX(angle);
                break;
            case ROT_Y:
                m = rotY(angle);
                break;
            case ROT_Z:
                m = rotZ(angle);
                break;
        }
        
        accum = accum * m;
    }
    
    return accum;
}

int main(int argc, char** argv)
{
    SaveFormat* save_format = &save_format_table[0];
    
    string input_filename;
    string output_filename;
    bool run_test = false;  // true if double rotate test is requested
    bool preview_mode = false;  // true when user wants quick result

    vector<RotType> rotation_sequence;
    rotation_sequence.push_back(ROT_X);
    rotation_sequence.push_back(ROT_Y);
    rotation_sequence.push_back(ROT_Z);
    
    vector<double> rotation_angles;     // in degrees
    size_t rotation_angles_specified;
    
    // rotation_matrix to use for performing the image rotation.
    // value updated after parsing arguments.
    Mat3 rotation_matrix = ident();
    
    ImageSaveParams save_params;
    
    
    if(argc <= 1)
    {
        print_usage();
        return 0;
    }
    
    for(int i = 1; i < argc; i++)
    {
        string arg = argv[i];
                
        if(arg == "-h" || arg == "--help" || arg == "-help")
        {
            print_usage();
            return 0;
        }
        
        if(arg == "--test" || arg == "-test")
        {
            run_test = true;
            continue;
        }
        
        if(arg == "--preview" || arg == "-preview")
        {
            preview_mode = true;
            continue;
        }
        
        if(arg == "--order" || arg == "-order")
        {
            i++;
            
            if(i >= argc)
            {
                fprintf(stderr, 
                    "[ERROR] Expected rotation order in arguments\n");
                return EXIT_FAILURE;
            }
            
            bool ok = parse_order(rotation_sequence, argv[i]);
            
            if(!ok)
            {
                fprintf(stderr, "[ERROR] Failed to parse rotation order\n");
                return EXIT_FAILURE;
            }
        }
        
        if(arg == "-i")
        {
            i++;
            
            if(i >= argc)
            {
                fprintf(stderr, 
                    "[ERROR] Expected input filename in arguments\n");
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
                fprintf(stderr, 
                    "[ERROR] Expected output filename in arguments\n");
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
                fprintf(stderr, 
                    "[ERROR] Expected output format after -f flag\n");
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
        
        if(arg == "-q")
        {
            i++;
            
            if(i >= argc)
            {
                fprintf(stderr, "[ERROR] Expected integer after -q flag\n");
                return EXIT_FAILURE;
            }
            
            save_params.quality = atoi(argv[i]);
            continue;
        }
        
        double angle = 0.0;
        if(sscanf(argv[i], "%lf", &angle))
        {
            rotation_angles.push_back(angle);
        }
    }
    
    
    // sanity check rotation angles/order and construct rotation
    rotation_angles_specified = rotation_angles.size();
    
    if(rotation_sequence.size() > rotation_angles.size())
    {
        fprintf(stderr, "[WARNING] Assuming unspecified angles are 0\n");
    }
    
    while(rotation_sequence.size() > rotation_angles.size())
    {
        rotation_angles.push_back(0.0);
    }
    
    if(rotation_sequence.size() < rotation_angles.size())
    {
        fprintf(stderr, "[WARNING] Extra angles are being ignored!\n");
    }
    
    rotation_matrix = make_rotation(rotation_sequence, rotation_angles);
    
    
    // sanity check file arguments and load input image
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
    
    ImageLoadResult load_result = load(src, input_filename);
    if(!load_result.ok)
    {
        fprintf(stderr, "[ERROR] Couldn't load input file -- stopping.\n");
        return EXIT_FAILURE;
    }
    
    // if test mode requested, run test and exit early
    if(run_test)
    {
        Mat3 rot;
        
        if(rotation_angles_specified == 0)
        {
            rot = rotX(deg2rad(90));
        }
        else
        {
            rot = rotation_matrix;
        }
        
        double_rotate_test(src, rot, preview_mode);
        return 0;
    }

    
    // print details about request for user sanity checking
    printf("Input:       %s\n", input_filename.c_str());
    printf("Output:      %s\n", output_filename.c_str());
    printf("Output type: %s\n", save_format->flag_name.c_str());
    printf("Size:        %lu %lu\n", src.width, src.height);

    printf("Order:       ");

    for(size_t i = 0; i < rotation_sequence.size(); i++)
    {
        switch(rotation_sequence.at(i))
        {
            case ROT_X:
                printf("Roll");
                break;
            
            case ROT_Y:
                printf("Pitch");
                break;
            
            case ROT_Z:
                printf("Yaw");
                break;
        }
        
        if(i != rotation_sequence.size()-1)
            printf(" -> ");
    }

    printf("\nAngles:      ");
    
    for(size_t i = 0; i < rotation_angles.size(); i++)
    {
        const char *w = "", *s;
        if(i >= rotation_angles_specified)
            w = "(auto)";
        else if(i >= rotation_sequence.size())
            w = "(!)";
        
        if(i != rotation_angles.size()-1)
            s = " ";
        else
            s = "";
        
        printf("%g%s%s", rotation_angles.at(i), w, s);
    }
    
    printf("\n");
    
    
    // actually process the image
    dst.resize(src.width, src.height);
    
    if(preview_mode)
    {
        printf("Preview mode enabled -- quality may be reduced to produce"
               " results faster\n");
               
        remap_fast(dst, src, rotation_matrix);
    }
    else
    {
        remap_full3(dst, src, rotation_matrix);
    }
    
    if(save_format->flag_name == "TIFF")
    {
        save_params.bps = load_result.bps;
        save_params.spp = load_result.spp;
    }
    
    save_format->save(dst, output_filename, save_params);
    
    return 0;
}

