#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "escher.h"

#define VERSION "0.99a (monochrome)"

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
int dpi = 300;
int rx;
static int aspect = 0;
int bands_per_screen = 8, maxx = 2560, maxy = 1600;
int maxcolour = 1;
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
int* id_line;
double wx,wy;   // wx,wy are the scaling factors for the pattern image
double sx,sy;   // sx,sy are the scaling factors for the source image
double pw;
int y;
unsigned source_y;
int lastpct = 0, bytecnt = 0;
const w = int (2.5*dpi+.5); // eye separation is approx 2.5 inches
const double r = 1/3.0;     // the depth field part of the picture

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
			return int ((N-n)*magnification);
		return 0;
		}

void strlower (char* p) {
	 for (; *p; p++) *p = tolower (*p);
	 }

template<class T> inline int round (T t) { return int (t+.5); }

int separation (double z) {
	return round ((1 - r * z) * w / (2 - r * z));
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
				 case '-': if (!strcmp (&arg [2], "help")) {
									 cout << "StereoScope v " << VERSION
									<< " - written by Maz Spork "
													 "(halgrim@diku.dk) 1994\n\nSyntax: " << argv [0]
                        << " [options] source_file pattern\n\n"
				 "Options are:\n"
				 "   -a        adjust high depth values negative\n"
				 "   -b #      number of vertical bands ["
					 << bands_per_screen << "]\n"
				 "   -h #      set height of output picture ["
					 << maxy << "]\n"
				 "   -i        invert depth information\n"
				 "   -m #      depth magnification ["
					 << magnification << "]\n"
				 "   -n        negative output picture\n"
				 "   -o name   output pic file name [output.ps]\n"
				 "   -r res    resolution (dpi) in output device ["
											 << dpi << "]\n"
				 "   -w #      set width of output picture ["
					 << maxx << "]\n"
				 "   -t name   give the picture a name [Untitled]\n"
				 "   -u        use unsigned (positive) depth only\n"
				 "   -x        maintain aspect ratio in pattern\n"
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
		 }
			 else return "illegal switch";
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
				 case 'r': argstream >> dpi;
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
   if (argc == 1) return "bad args, try option --help";
	 if (unsigned_depth && adjust_negative)
			return "options a and u are mutually exclusive";
   return 0;
   }

int main (int argc, char** argv) {
	 int i, j;

	 // read options and set variables, report errors in command line

	 char* rv = options (argc, argv);
	 if (rv) {
      if (*rv) cout << "error: " << rv << endl;
      exit (10);
      }

	 // open the output EPSF file

   psfile.open (output_file, ios::out | ios::binary);

	 // check the necessity of reading the pattern from a GIF file...

	 if (pattern == isFile) {
			ifstream pat_file (pattern_file, ios::in | ios::binary);
			if (!pat_file) {
				 cerr << "error: pattern file " << pattern_file
							<< " not found, exiting.\n";
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

	 // and the input depth information file

	 if (source_file) {
			input.open (source_file, ios::in | ios::binary);
			if (!input) {
				 cerr << "error: depth file "<<source_file<<" not found, exiting.\n";
				 exit (10);
				 }
			read_gif_header (input);
			}

	 // the rest of the initialisation

	 midx = maxx / 2, midy = maxy / 2;
	 maxd = sqrt (double(midx) * double(midx) + double(midy) * double(midy));

	 cout << "\nStereoScope v. " << VERSION
				<< " - (c) 1994 Maz Spork\n\n"
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
					 "   Res:     " << dpi << " dpi\n"
           "   Size:    " << ((long(maxx)*long(maxy))/256)*65+506+strlen(title)
                          << " bytes\n"
					 "   Bands:   " << bands_per_screen << "\n"
           "   Magnif:  " << magnification << "\n\n";

   // attempt to generate some mediocre postscript output
	 // the "bitdump" function takes three arguments; width, height
	 // and magnification. the bitmap is positioned at the current
   // origin, set by the translate command.

	 psfile << "%!PS-Adobe-1.0\n%%BoundingBox: 0 0 "
					<< maxx << " " << maxy
					<< "\n%%Creator: StereoScope v " << VERSION << " - Maz Spork\n"
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

	 sx = double (p_width) / double (maxx);
	 rx = round (1./sx);
	 sy = double (p_height) / double (maxy);
	 y = 0, source_y = 0;
	 pw = round (separation (0));
	 wx = pattern_width / double (pw);
	 wy = pattern_height / double (pw);

	 // allocate some work space for the scan lines &c.

	 if ((source_pattern = new char [pw]) == NULL) {
		 cerr << "allocation error: source pattern line\n";
		 exit (10);
	 }

	 if ((id_line = new int [maxx]) == NULL) {
		 cerr << "allocation error: identity links\n";
		 exit (10);
	 }

	 if ((scan_line = new char [maxx]) == NULL) {
		 cerr << "allocation error: scan line buffer\n";
		 exit (10);
	 }

	 // create the superimposition

	 gif_decode (DepthGeneration);

	 psfile << endl << "showpage" << endl << "%%Trailer" << endl;

	 cout << "\rFinished.      " << endl;

	 delete source_pattern;
	 delete scan_line;
   delete id_line;

   if (pattern == isFile) delete pattern_bitmap;

	 return 0;
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
   scaled *= (long double) (source_y + 1.0);
   return int (scaled + 0.5);
   }

// the next function is called from the GIF decoder every time a
// line in the source picture has been decoded. It will generate
// the appropriate number of output lines according to the scaling.

void DepthGeneration (char* vec, int) {

	// here are some constants:
  // w is the number of dots (physically) between two eye separations
  // r is the part of the depth field that can be seen. the higher we
	//   set r, the more difficult the lines will converge with respect
	//   to the viewer.

	int x, j;

	// we have several scaling problems involved here. first, the
	// source pattern needs to be scaled to the actual width of
	// each vertical band. then, the source picture line width needs
	// to be scaled according to the output file line width.

	double dx;

	// this following main loop is iterated either zero, one or more than
	// one time according to scale. this means that the global destination
	// picture's y coordinate is incremented until it reaches the next
	// scaled y computed from the next source y. this ensures that the
	// scaling will be evenly distributed over the entire y range.

	// the vector passed from the GIF decoder is taken to be 8-bit,
	// so we need some kind of normalization

	double* z = new double [p_width];

	// initialize the identity links and create the true z values

	for (x = 0; x < p_width; x++) {
		int h = vec [x];
		if (invert_depth) h = -h;                  // reverse depth
		if (unsigned_depth && h < 0)
			h += colours;                            // negative values -> positive
		id_line [x] = x, z [x] = h / 256.0;
		}

	for (x = 0; x < p_width; x++) {

		// this is the separation value in the picture plane of the
		// depth of the current coordinate.
		//
		//       1-rz
		//  s =  ---- w
		//       2-rz

		int s = separation (z [x]), left  = x - s / 2, right = left + s;

		if (left >= 0 && right < p_width) {
			int visible, t = 1;
			double zt;

			// check visibility --- hidden lines is searched for by similar
			// triangles until found or depth field exhausted using the
			// inequality
			//
			//        2(2-rz0)
			//   z0 + -------- t  <  z[t]     (or less than 1)
			//           rw

			do {
				zt = z[x] + 2 * (2 - r * z[x]) * t / (r * w);
				visible = z[x-t] < zt && z[x+t] < zt, t++;
				} while (visible && zt < 1);

			// last, juggle the pointers

			if (visible) {
				int l = id_line [left];
				while (l != left && l != right)
					if (l < right)
						left = l, l = id_line [left];
					else
						id_line [left] = right, left = right, right = l = id_line [left];
					id_line[left] = right;
					}
				}
			}

		// the following two loops facilitate the scaling from the depth source
		// to the desitination raster image: (p_width,p_height) -> (maxx,maxy)

		for (; y < next_y (); y++) {

			if (pattern == isFile)
				for (int xx = 0; xx < pw; xx++) {
					int tx = int (double (xx) * wx),
							ty = int (double (y) * (aspect?wx:wy));
					source_pattern [xx] =
						pattern_bitmap [ty%pattern_height*(pattern_width/8)+tx/8] &
							(1 << (tx%8)) ? maxcolour : 0;
					}

			for (int xx = maxx - 1; xx >= 0; xx--) {
				int tx = int (double (xx) * sx);
				if (id_line [tx] == tx) {  // new unconstrained value
					if (pattern == isRandom)
						scan_line [xx] = rand () % (maxcolour + 1);
					else if (pattern == isFile)
						scan_line [xx] = source_pattern [xx % int(pw)];
					}
				else
					scan_line [xx] = scan_line [round (id_line[tx]/sx)+xx%rx];
				}

			// this loop generates a scan line in ASCII format,
			// appropriate for EPSF format output. each line is
			// 256 pixels making them readable from most text editors.

			j = 0;
			for (xx = 0; xx < maxx; xx++) {
				if (!scan_line [xx] == negative) j |= (1 << (7 - (xx & 7)));
				if ((xx & 7) == 7 || xx == maxx - 1) {
				if (j < 16) psfile << "0";
				psfile << hex << j;
				j = 0;
				if (!(++bytecnt % 32)) bytecnt = 0, psfile << endl;
				}
			}
		}

	// before we leave the function for a single source picture
	// scan line, we increment the source y value and emit a
	// progres indication.

	source_y++;

	double dp = double (source_y) / double (p_height);
	int pct = int (100.*dp);
	if (lastpct != pct)
		cout << flush << "\r" << (lastpct = pct) << "% finished.";

	delete z;
	}








