#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <strstrea.h>
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
int negative = 0, colour = 0;
double magnification = 1.0;
int unsigned_depth = 1;          // always unsigned depth, no adjusting
int adjust_negative = 0;         // adjust high colours to negative values
int invert_depth = 0;            // invert depth information
int colours;
static int aspect = 0;
int bands_per_screen = 8, maxx = 2560, maxy = 1600;
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
char* pattern_bitmap = NULL;
char* source_pattern, *scan_line;
double wx,wy;   // wx,wy is the scaling factors for the pattern image
double sx,sy;   // sx,sy is the scaling factors for the source image
int y;
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
   static char* err1 = "missing argument for option ?";
   int i = 0, j = 0;  // physical arg index and non-option arg counter
   while (++i < argc) {
      char* arg = argv [i];
      if (*arg == '-' || *arg == '/') {
	 if (!arg [1]) return "switch character without option";
         char opt = tolower (arg [1]);
         switch (opt) {                     // no-argument options here
         case 'a': adjust_negative = !adjust_negative;
                   continue;
         case 'i': invert_depth = !invert_depth;
                   continue;
         case 'n': negative = !negative;
                   continue;
         case 'u': unsigned_depth = !unsigned_depth;
                   continue;
         case 'x': aspect = 1;
                   continue;
         case 'q': cout << "StereoScope v. 0.02 - written by Maz Spork "
                           "(halgrim@diku.dk) 1994\n\nSyntax: " << argv [0]
                        << " [options] source_file pattern\n\n"
			   "Options are:\n"
			   "   -a        adjust high depth values to negative\n"
			   "   -b #      number of vertical bands ["
					 << bands_per_screen << "]\n"
			   "   -h #      set height of output picture ["
					 << maxy << "]\n"
			   "   -i        invert depth information\n"
			   "   -m #      depth magnification ["
					 << magnification << "]\n"
			   "   -n        negative output picture\n"
			   "   -o name   output picture file name [output.ps]\n"
			   "   -w #      set width of output picture ["
					 << maxx << "]\n"
			   "   -t name   give the picture a name [Untitled]\n"
			   "   -u        use unsigned (positive) depth only\n"
			   "   -x        maintain aspect ratio in pattern image\n"
			   "\nThe pattern file contains the background bitmap "
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
         if (i == argc || !*arg || *arg == '-' || *arg == '/') {
            err1 [strlen (err1) - 1] = opt;
            return err1;
            }
         strlower (arg);
         istrstream argstream (arg, strlen (arg));
         switch (opt) {                  // argument options here
         case 'o': output_file = arg;
                   break;
         case 'b': argstream >> bands_per_screen;
                   break;
         case 'w': argstream >> maxx;
                   break;
         case 'h': argstream >> maxy;
                   break;
         case 't': title = arg; *arg = toupper(*arg);
                   break;
         case 'm': argstream >> magnification;
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
         cerr << "error: pattern file " << pattern_file << " not found, exiting.\n";
         exit (10);
         }
      read_gif_header (pat_file);
      pattern_height = p_height, pattern_width = p_width;
      if ((pattern_width % 8) != 0)
         cout << "warning: pattern bitmap width not multiplum of 8...\n";
      unsigned sz = ((pattern_width + 7) / 8) * pattern_height;
      if ((pattern_bitmap = new char [sz]) == NULL) {
         cerr << "allocation error: background pattern bitmap\n";
         exit (10);
         }
      for (int i = 0; i < sz; i++) pattern_bitmap [i] = 0;
      pat_y = 0;
      gif_decode (PatternGeneration);
      }
   else if (pattern == isEscher) {
      pattern_height = bitmap_height,
      pattern_width  = bitmap_width,
      pattern_bitmap = bitmap;
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

   cout << "\nStereoScope v. 0.8 - (c) 1994 Maz Spork (monochrome version)\n\n"
           "Input file information:\n"
           "   Depth:   GIF file `" << source_file << "' ("
        << p_width << "x" << p_height << ", " << colours << " colours)\n";
   if (unsigned_depth)
      cout << "            - always positive, towards viewer\n";
   if (adjust_negative)
      cout << "            - adjusted for negative values\n";
   if (invert_depth)
      cout << "            - inverted, concave <-> convex\n";
   cout << "   Pattern: ";

   if (pattern == isRandom) cout << "built-in (random pixels)\n";
   else if (pattern == isFile) cout << "GIF file `" << pattern_file <<
                    "' (" << pattern_width << "x" << pattern_height << ")\n";
   else cout << "built-in monochrome image (Escher's Knot)\n";
   if (aspect)
      cout << "            - aspect ratio is maintained\n";
   cout << "Output file information:\n"
           "   Image:   " << "EPS file `" << output_file << "' ("
        << maxx << "x" << maxy << ", " << 1+maxcolour << " colours)";
   if (negative) cout << " (negative)"; cout << "\n"
           "   Size:    " << ((long(maxx)*long(maxy))/256)*65+506+strlen(title) << " bytes\n"
           "   Bands:   " << bands_per_screen << "\n"
           "   Magnif:  " << magnification << "\n\n";

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
          << 1 << " bitdump" << endl << endl;

   // allocate some work space for the scan lines

   if ((source_pattern = new char [pixels_per_band]) == NULL) {
      cerr << "allocation error: source pattern line\n";
      exit (10);
      }

   if ((scan_line = new char [maxx]) == NULL) {
      cerr << "allocation error: scan line buffer\n";
      exit (10);
      }

   // work out the scaling factors - these transform coordinates in the
   // pattern image to coordinates in the source pattern and coordinates
   // in the destination picture to coordinates in the depth picture.

   if (pattern == isFile)
      wx = double (pattern_width) / double (pixels_per_band),
      wy = double (pattern_height) / double (pixels_per_band);

   sx = double (p_width) / double (maxx);
   sy = double (p_height) / double (maxy);
   y = 0, source_y = 0;

   // create the superimposition

   gif_decode (DepthGeneration);

   psfile << endl << "showpage" << endl << "%%Trailer" << endl;

   cout << "\rFinished.      " << endl;

   delete source_pattern;
   delete scan_line;
   if (pattern == isFile) delete pattern_bitmap;

}

// read the pattern file from a GIF image (preferably monochrome)
// this function is called every time a scan line has been read
// from the pattern file, and each line is copied to the bitmap
// buffer, where it sits - accumulating interest (or whatever).

void PatternGeneration (char* vec, int) {
   char* current_line = pattern_bitmap + pat_y++ * ((pattern_width+7) / 8);

   for (int i = 0; i < pattern_width; i++)
      if (vec [i]) current_line [i/8] |= (1 << ((i%8)));
   }

// this function computes the next y-value in the destination picture,
// that is the scaled value for the next y-value in the source picture
// used as the end condition for the main loop in DepthGeneration. Note
// that a precision of 80 bits IEEE is necessary, as the scaling factor
// can get as low as 2000/1999 and needs to be precise over the entire
// y-range of 0..1999.

inline int next_y () {
   long double scaled = (long double) (maxy)/(long double) (p_height);
   scaled = scaled * (long double) (source_y + 1.0);
   return int (scaled);
   }

// the next function is called from the GIF decoder every time a
// line in the source picture has been decoded. It will generate
// the appropriate number of output lines according to the scaling.

void DepthGeneration (char* vec, int) {

   int x, j;

   // we have several scaling problems involved here. first, the
   // source pattern needs to be scaled to the actual width of
   // each vertical band. then, the source picture line width needs
   // to be scaled according to the output file line width.

   double dx;

   // this following main loop is iterated either zero, one or more than
   // once according to scale. this means that the global destination
   // picture's y coordinate is incremented until it reaches the next
   // scaled y computed from the next source y. this ensures that the
   // scaling will be evenly distributed over the entire y range.

   for (; y < next_y (); y++) {

      // initialise the source pattern (random, built-in, or from a file)

      if (pattern == isRandom)
         for (x = 0; x < pixels_per_band; x++)
            source_pattern [x] = rand () % (maxcolour + 1);
      else {

      // the following loop involves sub-scaling of the pattern image
      // from the wx,wy scaling factors. wx and wy may be lower, equal to
      // or more than 1. if the aspect ratio option is set, the y scaling
      // is computed using the wx scaling factor, otherwise the y scaling
      // factor is used (and thus the pattern pictures will be square).

         for (x = 0; x < pixels_per_band; x++) {
            int tx = double (x) * wx, ty = double (y) * (aspect ? wx : wy);
            source_pattern [x] =
               pattern_bitmap [ty%pattern_height*(pattern_width/8)+tx/8] &
                  (1 << (tx%8)) ? maxcolour : 0;
            }
         }

      // the next loop copies the pattern into the output scan line
      // buffer for the entire line width.

      for (x = 0; x < maxx; x++)
	 scan_line [x] =
	    source_pattern [(x - pixels_per_band / 2) % pixels_per_band];

      // then we do the right-hand side of the picture. the function
      // used is something like
      //    v[x]=v[x-w+height(v[x])*magnification]
      // where w is pixels per band in the destination picture. note
      // that this is a surjective function. the scaling involved
      // here is a/b where a is the physical dimensions of the depth
      // picture divided by the physical dimensions of the output
      // picture.

      for (x = (maxx + pixels_per_band) / 2; x < maxx; x++) {
         int tx = double (x - pixels_per_band / 2) * sx;
         int h = vec [tx];
         if (invert_depth) h = -h;
         if (unsigned_depth && h < 0)
            h+= colours;  // adjust negative values to positive
         int px = int (x - pixels_per_band + h * magnification);
         scan_line [x] = scan_line [px];
         }

      // then we do the left-hand side of the picture. the
      // function used is
      //    v[x+height(v[x+w])*magnification]=v[x+w]
      // which is not surjective. thus we cheat and fill out
      // the missing pixels with adjacent pixels from the right
      // hand pattern band. it's dirty, but it seems to work.
      // the scaling involved is similar to the above loop.

      int lastqx, tempqx;

      for (lastqx = x = (maxx - pixels_per_band) / 2; x >= 0; x--) {
         int px = x + pixels_per_band;
         int tx = double (x + pixels_per_band / 2) * sx;
         int h = vec [tx];
         if (invert_depth) h = -h;
         if (unsigned_depth && h < 0)
            h+= colours;  // adjust to positive values
         int qx = int (x + h * magnification);
         tempqx = lastqx; if (qx < lastqx) lastqx = qx; // remember highest
         do { scan_line [qx++] = scan_line [px++];
            } while (qx < tempqx);
         }

      // this loop generates a scan line in ASCII format,
      // appropriate for EPSF format output. each line is
      // 256 pixels making them readable from a text editor.

      for (j = x = 0; x < maxx; x++) {
         if (!scan_line [x] == negative) j |= (1 << (7 - (x & 7)));
         if ((x & 7) == 7 || x == maxx - 1) {
            if (j < 16) psfile << "0";
            psfile << hex << j;
            j = 0;
            if (!(++bytecnt % 32)) bytecnt = 0, psfile << endl;
            }
         }
      }

   // before we leave the function for a single source picture
   // scan line, we increment the source y value and emit a
   // progres indicator.

   source_y++;

   int pct = double (source_y) / double (p_height) * 100;
   if (lastpct != pct) {
      cout << "\r" << (lastpct = pct) << "% finished.";
      }
   }



