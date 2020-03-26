Usage: panorotate -i <filename> [-o <filename>] [-f <format>]
                  [-q <jpg_quality>] [--test] [<x>] [<y>] [<z>]

Flags and arguments:
    -i filename    specify input filename (required)
    -o filename    specify output filename (required except for --test)
    -f format      specify output format (default is 'TIFF')
                   See the table below for full list of possible formats.
    -q jpg_quality Integer quality percent to use when saving as JPEG.
                   (default is 90)
    --test         Run the double rotation test and print statistics.
    <x y z>        Rotation angles in degrees (unset values default to 0)

Example usage:
    panorotate -i input.tif -o output.tif -f TIFF_RGBA16 90 0 0

    This loads input.tif, rotates the X axis by 90 degrees, and saves the
    output into output.tiff in 16-bit RGBA format.

Recognized save format flags (for -f):
    TIFF           (default) -- matches bps/spp from input
    JPG            8-bit RGB JPEG (default q=90)
    JPEG           (synonym; same as above)
    TIFF_RGB8      8-bit  RGB  TIFF
    TIFF_RGB16     16-bit RGB  TIFF
    TIFF_RGBA8     8-bit  RGBA TIFF
    TIFF_RGBA16    16-bit RGBA TIFF

