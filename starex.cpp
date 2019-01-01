#include <graphics.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>

template <class T> T round (T t) { return (T) (int (t + 0.5)); }

const int dpi = 75, rows_per_screen = 6;
const double e = 2.5, pi2 = 2 * 3.1415926, pictures = 16,
   w = round (e * dpi), r = 1/3.;             
double asp;            
int maxx, maxy, midx, midy, maxcolour, row_height;        
unsigned block_size;    

int separation (double z) {
   return round ((1 - r * z) * w / (2 - r * z)); 
   }

double depth (int x, int y, int t) {
   double dx = abs (midx - x), dy = abs (midy - y) * asp,
	  d = sqrt (dx * dx + dy * dy), // afstand fra centrum
	  h = cos (d / (maxx / 25) + (1 - (t / pictures)) * pi2) / 2 + 0.5;
   return h;
   }

int colour (int, int, int) {
   return random (maxcolour);
   }

void generate_images () {
   ofstream image_output;   
   char* scan_line, * graph_buffer, filename [13];   

   if (((graph_buffer = new char [block_size]) == NULL) ||
       ((scan_line = new char [maxx]) == NULL)) {
	  cerr << "Lagerallokeringsfejl\n"; exit (2); }

   for (int picture = 0; picture < pictures; picture++) {
      srand (42);
      sprintf (filename, "IMAGE%02d.3D", picture);
      image_output.open (filename, ios::out | ios::binary);
      if (!image_output) {
	 cerr << "Filallokeringsfejl: " << filename << endl; exit (3); }

      for (int y = 0, row = 0; row < rows_per_screen; row++) {
	 for (int dy = 0; dy < row_height; dy++, y++) {
	    for (int x = 0; x < separation (0); x++)
	       scan_line [x] = colour (x, y, picture);
	    for (; x < maxx; x++)
	       scan_line [x] = scan_line
		  [x - separation (depth (x, y, picture))];
	    for (x = 0; x < maxx; x++) putpixel (x, y, scan_line [x]);
	    }
	 getimage (0, row * row_height, maxx, y, graph_buffer);
	 image_output.write (graph_buffer, block_size);
	 }
      image_output.close ();
      }
   delete scan_line;
   delete graph_buffer;
   }

void animate_images () {
   ifstream image_input;
   int c;
   char* graph_buffer, filename [13];

   if ((graph_buffer = new char [block_size]) == NULL) {
      cerr << "Lagerallokeringsfejl\n"; exit (2); }

   while (c != 'q') {
      for (int picture = 0; c != 'q' && picture < pictures; picture++) {
	sprintf (filename, "IMAGE%02d.3D", picture);
	image_input.open (filename, ios::in | ios::binary);
	setactivepage (1 & picture);
	for (int row = 0; c != 'q' && row < rows_per_screen; row++) {
	   image_input.read (graph_buffer, block_size);
	   putimage (0, row * row_height, graph_buffer, COPY_PUT);
	   if (kbhit ()) {
	      c = getch () | 0x20;
	      if (c == 'h') getch ();
	      }
	   }
	setvisualpage (1 & picture); 
	image_input.close();
	}
      }
   delete graph_buffer;
   }

void main (void) {
   int xasp, yasp, gdriver = EGA64, gmode = EGAHI, errorcode;
   initgraph (&gdriver, &gmode, "");
   if ((errorcode = graphresult ()) != grOk) {
      cerr << "Grafikfejl: " << grapherrormsg (errorcode) << endl; exit (1); }
   maxx = getmaxx (), maxy = getmaxy ();
   midx = (maxx + separation (0)) / 2;
   midy = (maxy + separation (0)) / 2;
   row_height = maxy / rows_per_screen;
   maxcolour = getmaxcolor ();
   block_size = imagesize (0, 0, maxx, row_height);
   getaspectratio (&xasp, &yasp); asp = double (yasp) / double (xasp);
   generate_images ();
   animate_images ();
   closegraph ();
   }


