#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <conio.h>
#include <math.h>
#include <ctype.h>
#include "escher.h"

extern char bitmap [bitmap_width/8 * bitmap_height];

enum { isRandom, isEscher, isFile } pattern = isRandom;

char* output_file = "output.ps", *source_file = 0, *pattern_file = 0;
char* title = "Untitled";
int standard_height (int, int);
int (*height) (int, int) = 0;
int negative = 1, colour = 0, magnification = 1;
int unsigned_depth = 0;          // always unsigned depth, no adjusting
int adjust_negative = 0;         // adjust high colours to negative values
int invert_depth = 0;            // invert depth information
int colours;
int bands_per_screen = 6, maxx = 640, maxy = 400;
int pixels_per_band, maxcolour = 1;
int midx, midy;
double maxd;
static ifstream input;
static ofstream psfile;
void DepthGeneration (char*,int);
void PatternGeneration (char*,int);
extern void read_gif_header (istream&);
extern void gif_decode (void (*) (char*,int));
extern unsigned int p_width, p_height;
unsigned pattern_height, pattern_width, pat_y;
char* pattern_bitmap = 0;
char* source_pattern, *scan_line;
double wx,wy;   // wx,wy is the scaling factors for the pattern image
double sx,sy;   // sx,sy is the scaling factors for the source image
double y;
unsigned source_y;
int lastpct = 0, bytecnt = 0;

// stub zero height function - a very boring function

int standard_height (int, int) {
   return 0;
   }


// raindrop

int raindrop_height (int x, int y) {
   double dx = abs (midx - x), dy = abs (midy - y);
   double d = sqrt (dx * dx + dy * dy);
   double h = (1 + cos (d / (maxx/25))) * magnification * 1/(1+(d/maxx));
   return int (h);
   }

// pyramid

int pyramid_height (int x, int y) {
    const N = 12;   /* no. of steps in pyramid */
    for (int n = 1; n<=N; n++)
        if (x > (maxx/2 - (maxx/N)*n) && x < (maxx/2 + (maxx/N)*n)
        &&  y > (maxy/2 - (maxy/N)*n) && y < (maxy/2 + (maxy/N)*n))
            return (N-n)*magnification;
    return 0;
    }

void strlower (char* p) {
   for (; *p; p++) *p = tolower (*p);
   }

// function options parses the command line, sets various options
// accordingly and returns possible errors. If a NULL pointer is
// returned, errors are but none, if the string is a null string,
// the program should terminate without complaining and if a
// static string is returned it comprises an error message which
// can be displayed with extra information before terminating.

char* options (int argc, char** argv) {
   int i = 0, j = 0;  // physical arg index and non-option arg counter
   while (++i < argc) {
      char* arg = argv [i];
      if (*arg == '-' || *arg == '/') {
         if (!arg [1]) return "switch character without option";
         char opt = tolower (arg [1]);
         switch (opt) {                     // no-argument options here
         case 'a': adjust_negative = 1;
                   continue;
         case 'i': invert_depth = 1;
                   continue;
         case 'n': negative = !negative;
                   continue;
         case 'u': unsigned_depth = 1;
                   continue;
         case '?': cout << "StereoScope v. 0.01 - written by Maz Spork "
                           "(halgrim@diku.dk) 1994\n\nSyntax: " << argv [0]
                        << " [options] source_file pattern\n\n"
                           "Options are:\n"
                           "   -a        adjust high depth values to negative\n"
                           "   -b #      number of vertical bands\n"
                           "   -h #      set height of output picture\n"
                           "   -i        invert depth information\n"
                           "   -m #      depth magnification\n"
                           "   -n        negative output picture\n"
                           "   -o name   output picture file name\n"
                           "   -w #      set width of output picture\n"
                           "   -t name   give the picture a name\n"
                           "   -u        use unsigned (positive) depth only\n"
                           "\nThe pattern file contains the background bitmap"
                           "for the stereo image\nas a GIF file, except when "
                           "given as\n"
                           "   random         use a random pattern\n"
                           "   escher         use a built-in bitmap\n"
                           "\nThe source file contains the depth information "
                           "for the output\npicture, except when specified as"
                           "\n   pyramid        generate a pyramidic 3D image"
                           "\n   raindrop       generate a `rain-drop'-effect"
                        << endl;
                   return "";
         default: break;                  // break case with continue
         }
         arg = argv [i] [2] ? &argv [i] [2] : argv [++i];
         if (i == argc || !*arg) return "missing argument for option";
         strlower (arg);
         switch (opt) {                  // argument options here
         case 'o': output_file = arg;
                   break;
         case 'b': bands_per_screen = atoi (arg);
                   break;
         case 'w': maxx = atoi (arg);
                   break;
         case 'h': maxy = atoi (arg);
                   break;
         case 't': title = arg;
                   break;
         case 'm': magnification = atoi (arg);
                   break;
         default:  return "illegal switch";
            }
         }
      else {    // it's not an option - must be a direct argument
         j++;
         strlower (arg);
         switch (j) {
         case 1: if (!strcmp (arg, "pyramid"))
                    height = pyramid_height;
                 else if (!strcmp (arg, "raindrop"))
                    height = raindrop_height;
                 else source_file = arg, height = 0;
                 continue;
         case 2: if (!strcmp (arg, "random"))
                    pattern = isRandom;
                 else if (!strcmp (arg, "escher"))
                    pattern = isEscher;
                 else pattern = isFile, pattern_file = arg;
                 continue;
         default: return "too many arguments";
            }
         }
      }
   if (!source_file && !height) return "you must specify a source file name";
   if (argc == 1) return "bad args, try option -?";
   if (unsigned_depth && adjust_negative)
      return "options a and u are mutually exclusive";
   return 0;
   }

void main(int argc, char** argv)
{
   int i, j;

   // read options and set variables, report errors in command line

   char* rv = options (argc, argv);
   if (rv) {
      if (*rv) cout << "error: " << rv << endl;
      exit (10);
      }

   // open the output EPSF file

   psfile.open (output_file, ios::binary);

   // check the necessity of reading the pattern from a GIF file...

   if (pattern == isFile) {
      ifstream pat_file (pattern_file, ios::binary);
      if (!pat_file) {
         cerr << "error: pattern file " << source_file << " not found, exiting.\n";
         exit (10);
         }
      read_gif_header (pat_file);
      pattern_height = p_height, pattern_width = p_width;
      unsigned sz = ((pattern_width + 7) / 8) * pattern_height;
      pattern_bitmap = new char [sz];
      for (int i = 0; i < sz; i++) pattern_bitmap [i] = 0;
      pat_y = 0;
      gif_decode (PatternGeneration);
      }

   // and the input GIF file

   if (source_file) {
      input.open (source_file, ios::binary);
      if (!input) {
         cerr << "error: depth file " << source_file << " not found, exiting.\n";
         exit (10);
         }
      read_gif_header (input);
      }

   // the rest of the initialisation

   midx = maxx / 2, midy = maxy / 2;
   pixels_per_band = maxx / bands_per_screen;
   maxd = sqrt (double(midx) * double(midx) + double(midy) * double(midy));

   cout << "\nStereoScope v. 0.01 - (c) 1994 Maz Spork\n\n"
           "Generating stereo image to `" << output_file << "'.\n"
           "Depth data in GIF file `" << source_file << "', "
        << p_width << "x" << p_height << ", " << colours << " colours\n";

   if (pattern == isRandom) cout << "with random background pattern";
   else if (pattern == isEscher) cout << "with Escher's Knot as pattern";
   else
      cout << "with background pattern from GIF file `" << pattern_file <<
              "', " << pattern_height << "x" << pattern_width;

   cout << ".\nOutput file dimensions: " << maxx << "x" << maxy << ".\n"
           "Output format is Encapsulated PostScript.\n"
           "There are " << bands_per_screen << " vertical repeating images "
           "in this picture.\nColour depth: " << maxcolour << "\n" <<
           "Magnification: " << magnification << "\n";

   if (unsigned_depth)
      cout << "Depth information is one-way (descending)\n";
   if (adjust_negative)
      cout << "Depth information is adjusted for negative values\n";
   if (invert_depth)
      cout << "Depth information is inverted\n";

   // attempt to generate some mediocre postscript output
   // the "bitdump" function takes three arguments; width, height
   // and magnification. the bitmap is positioned at the current
   // origin, set by the translate command.

   psfile << "%!PS-Adobe-1.0\n%%BoundingBox: 0 0 "
          << maxx << " " << maxy
          << "\n%%Creator: StereoScope v 0.01 - Maz Spork\n"
          << "%%Title: " << title << "\n%%CreationDate: n/a\n%%EndComments"
             "\n%%Pages: 1\n%%EndProlog\n%%Page: 1 1\n\n"
             "/bitdump\n{\n /iscale exch def\n /height exch def\n "
             "/width exch def\n width iscale mul height iscale mul "
             "scale\n /picstr width 7 add 8 idiv string def\n width "
             "height 1 [width 0 0 height neg 0 height]\n { currentfile "
             "picstr readhexstring pop }\n image\n} def\n\n"
             "72 300 div dup scale\n2000 500 translate\n90 rotate\n"
          << maxx << " " << maxy << " "
          << 4 << " bitdump" << endl << endl;

   // allocate some work space for the scan lines

   source_pattern = new char [pixels_per_band];
   scan_line = new char [maxx];

   // work out the scaling factors - these transform coordinates in the
   // pattern image to coordinates in the source pattern and coordinates
   // in the destination picture to coordinates in the depth picture.

   if (pattern == isEscher || pattern == isRandom)
      wx = double (bitmap_width) / double (pixels_per_band),
      wy = double (bitmap_height) / double (pixels_per_band);
   else if (pattern == isFile)
      wx = double (pattern_width) / double (pixels_per_band),
      wy = double (pattern_height) / double (pixels_per_band);

   sx = double (p_width) / double (maxx);
   sy = double (p_height) / double (maxy);
   y = 0;
   source_y = 0;

   // create the superimposition

   gif_decode (DepthGeneration);

   psfile << endl << "showpage" << endl << "%%Trailer" << endl;

   delete source_pattern;
   delete scan_line;
}

// read the pattern file from a GIF image (preferably monochrome)

void PatternGeneration (char* vec, int) {
   char* current_line = pattern_bitmap + pat_y * ((pattern_width+7) / 8);

   for (int i = 0; i < pattern_width; i++)
      if (vec [i]) current_line [i/8] |= (1 << ((i%8)));

   pat_y++;
   }

// do the stuff...

void DepthGeneration (char* vec, int) {

   int x, j;

   // we have several scaling problems involved here. first, the
   // source pattern needs to be scaled to the actual width of
   // each vertical band. then, the source picture line width needs
   // to be scaled according to the output file line width.

   double dx;

   for (; y < (source_y + 1.) * (double(maxy)/double(p_height)); y++) {

      // initialise the source pattern

      if (pattern == isRandom)
         for (x = 0; x < pixels_per_band; x++)
            source_pattern [x] = rand () % (maxcolour + 1);
      else if (pattern == isEscher)
         for (x = 0; x < pixels_per_band; x++) {
            int tx = double (x) * wx, ty = double (y) * wy;
            source_pattern [x] =
               bitmap [ty%bitmap_height*(bitmap_width/8)+tx/8] &
                  (1 << (tx%8)) ? maxcolour : 0;
               }
      else if (pattern == isFile) {
         for (x = 0; x < pixels_per_band; x++) {
            int tx = double (x) * wx, ty = double (y) * wy;
            source_pattern [x] =
               pattern_bitmap [ty%pattern_height*(pattern_width/8)+tx/8] &
                  (1 << (tx%8)) ? maxcolour : 0;
            }
         }

      for (x = 0; x < maxx; x++) {
         int tx = double (x) * sx, ty = double (y) * sy, h;

         if (height)   // height information provided by function
            h = height (tx, ty);
         else {
            h = vec [tx];
            if (invert_depth) h = -h;
            if (adjust_negative && h > colours / 2) h -= colours;
            }

         h *= magnification;

         if (unsigned_depth) {
            unsigned uh = unsigned (h);
            if (x + uh < pixels_per_band)
               scan_line [x] = source_pattern [x + uh];
            else
               scan_line [x] = scan_line [x - pixels_per_band + uh];
            }
         else {
            if (x + h < pixels_per_band)
               scan_line [x] = source_pattern [x + h];
            else
               scan_line [x] = scan_line [x - pixels_per_band + h];
            }
      }

      for (j = x = 0; x < maxx; x++) {  // generate a scan line in ASCII
         if (!scan_line [x] == negative) j |= (1 << (7 - (x & 7)));
         if ((x & 7) == 7 || x == maxx - 1) {
            if (j < 16) psfile << "0";
            psfile << hex << j;
            j = 0;
            if (!(++bytecnt % 32)) bytecnt = 0, psfile << endl;
            }
         }
      } // y-coordinates within a single source picture scan line

   source_y++;

   int pct = double (source_y) / double (p_height) * 100;
   if (lastpct != pct) cout << "\r" << (lastpct = pct) << "% finished.";

   }
