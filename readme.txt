Usage: panorotate -i <filename> [-o <filename>] [-f <format>]
                  [-q <jpg_quality>] [--test] [--preview]
                  [--order <rpy>] [<angles...>]

Flags and arguments:
    -i filename    specify input filename (required)
    -o filename    specify output filename (required except for --test)
    -f format      specify output format (default is 'TIFF')
                   See the table below for full list of possible formats.
    -q jpg_quality Integer quality percent to use when saving as JPEG.
                   (default is 90)
    --test         Run the double rotation test and print statistics.
                   If no angles are specified by default a 90 degree
                   roll is used to test the quality.
    --preview      Perform a single sample per output pixel to create
                   a preview image more quickly.
    --order rpy    Rotation sequence to perform indicating order of
                   roll (R), pitch (P), and yaw (Y). Only 'R', 'P', 'Y'
                   may be used in the argument following --order, though
                   any number or combination of them may be provided.
                   e.g. R, RPY, RPR... if this argument is not
                   specified then the default order is RPY.
                   'RPY' means that first a roll will be performed, then
                   a pitch, and finally a yaw.
    [<angles...>]  Rotation angles in degrees.
                   Unspecified angles will be assumed to be zero.
                   Extra angles will be ignored.

Example usage:
    panorotate -i input.tif -o output.tif -f TIFF_RGBA16 90 0 0

    This loads input.tif, rotates the X axis by 90 degrees, and saves the
    output into output.tif in 16-bit RGBA format.

Recognized save format flags (for -f):
    TIFF           (default) -- matches bps/spp from input
    JPG            8-bit RGB JPEG (default q=90)
    JPEG           (synonym; same as above)
    TIFF_RGB8      8-bit  RGB  TIFF
    TIFF_RGB16     16-bit RGB  TIFF
    TIFF_RGBA8     8-bit  RGBA TIFF
    TIFF_RGBA16    16-bit RGBA TIFF

