// stare.cpp
//
// generering og animering af SIRDS-billeder - skal overs‘ttes med
// Borland's BGI-bibliotek, men kan overs‘ttes med andre produkter.
// bem‘rk, at der kr‘ves en C++ standard 2.1 eller h›jere.
//
// programmet udnytter Borland's grafikbibliotek og anvender funktioner
// til at l‘se og skrive til videolageret p† en meget forenklet (latterlig)
// m†de. du kan for›ge animationshastigheden meget ved selv at skrive et
// drivprogram til at l‘se fra og skrive til sk‘rmens lager.
//
// (C) 1994 Maz Spork, alle rettigheder forbeholdt forfatteren

#include <graphics.h>
#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>

// generisk afrundingsfunktion
template <class T> T round (T t) { return (T) (int (t + 0.5)); }

// programmet anvender en r‘kke konstanter til at skabe de rette
// foruds‘tninger for 3D-billederne (se artiklen)

int dpi = 75,                    // omtrentlig opl›sning p† en EGA-sk‘rm
	 rows_per_screen = 6;          // antal r‘kker p† sk‘rmen (sk›nsm‘ssigt)
double e = 2.5,                  // der er ca. 2.5" mellem ›jnene
   pi2 = 2 * 3.1415926,          // vi har brug for 2*pi
   pictures = 16,                // vores animation har 16 billeder
   w = round (e * dpi),          // antal pixels mellem dine ›jne
	 r = 1/3.;                     // dybdefeltet er 1/3 af fokusomr†de
																 // bag sk‘rmen (s‘t ned, hvis det er
																 // sv‘rt at for dig at fokusere n‘rt)

double asp;                      // aspekt-ratio s† vi f†r cirkler
int maxx, maxy,                  // sk‘rmens dimension (i pixels)
		midx, midy,                  // sk‘rmens midtpunkt
		maxcolour,                   // antal farver p† sk‘rmen - 1
    row_height;                  // h›jden af hver r‘kke, der kan hentes
				 // med BGI-funktionen getimage
unsigned block_size;             // st›rrelsen p† en blok (i byte)

// f›lgende funktion returnerer separationen p† sk‘rmen givet et
// z-koordinat et sted p† sk‘rmen (se figur)
//
//                    1-rz
//    separation(z) = ---- w
//                    2-rz

int separation (double z) {
   return round ((1 - r * z) * w / (2 - r * z));  // "hjertet" i SIRDS
   }

// f›lgende funktion beskriver dybden i billedet, den kaldes med et (x,y)-
// koordinat relativt fra ›verste venstre hj›rne i billedet samt en
// tidsangivelse. den returnerer dybden i punktet (x,y) til tiden t
// i intervallet [0..1].
// du kan skrive en anden funktion her eller eventuelt l‘se dybden ind
// fra en fil for at lave andre 3D-billeder. Hvad med en 3D-fraktal?

double depth (int x, int y, int t) {

   // f›lgende s‘tning genererer en regndr†be-effekt set fra oven,
   // hvor b›lgetoppene udg†r fra centrum.

   double dx = abs (midx - x), dy = abs (midy - y) * asp,
	  d = sqrt (dx * dx + dy * dy), // afstand fra centrum
	  h = cos (d / (maxx / 25) + (1 - (t / pictures)) * pi2) / 2 + 0.5;

   return h;
   }

// denne funktion beskriver farven (den fysiske) i billedpunktet
// (x,y) til tiden t. Her returneres blot en tilf‘ldig farve
// fra en r‘kke med fast udgangspunkt (fast "seed"), men du kan
// ‘ndre den til at tegne et m›nster eller et andet billede.

int colour (int, int, int) {
   return random (maxcolour);
   }

// den n‘ste funktion laver 3D-billederne og gemmer dem p† disken.

void generate_images () {
	 ofstream image_output;     // her skriver vi billederne
	 char* scan_line,           // her gemmer vi en sk‘rmlinie
			 * graph_buffer,        // og en grafikblok som holdeplads
	 filename [13];       // og her er plads til et DOS-filnavn...

	 if (((graph_buffer = new char [block_size]) == NULL) ||
			 ((scan_line = new char [maxx]) == NULL)) {
		cerr << "Lagerallokeringsfejl\n";
		exit (2);
		}

	 // denne l›kke genererer alle billederne ‚t for ‚t
	 for (int picture = 0; picture < pictures; picture++) {

			// der benyttes et fast udgangspunkt for farverne i billedet
			srand (42);

			// filerne hedder IMAGE00.3D, IMAGE01.3D, IMAGE02.3D ...
			sprintf (filename, "IMAGE%02d.3D", picture);
			image_output.open (filename, ios::out | ios::binary);
			if (!image_output) {
			cerr << "Filallokeringsfejl: " << filename << endl;
			exit (3);
			}

			// de n‘ste 15 linier laver 3D-effekten og gemmer billederne
			// i sm†bidder p† str›mmen "image_output". sm†bidderne er
			// n›dvendige, da Borland's grafikbibliotek er lidt forkrampet,
			// da det kun tillader, at vi flytter blokke under 64K.

		for (int y = 0, row = 0; row < rows_per_screen; row++) {
			 for (int dy = 0; dy < row_height; dy++, y++) {

			// f›rst laves et referencebillede til venstre ›je
			// som baggrundsbillede, dvs. et fladt stykke.

					for (int x = 0; x < separation (0); x++)
						 scan_line [x] = colour (x, y, picture);

			// derefter genereres alle h›jre-›je-billederne med
			// reference til venstre-›je-billederne. udregningerne
			// her er st‘rkt approksimerede og tager udgangspunkt
			// i konstanterne i starten af programmet.

					for (; x < maxx; x++)
						 scan_line [x] = scan_line
								[x - separation (depth (x, y, picture))];

				// til sidst tegnes de p† sk‘rmen

					for (x = 0; x < maxx; x++)
						 putpixel (x, y, scan_line [x]);
				}

				if (kbhit ())
					if (getch () == 'q') goto ulykke;

				// her skrives en billedramme til den aktuelle fil
				getimage (0, row * row_height, maxx, y, graph_buffer);
				image_output.write (graph_buffer, block_size);
				}
			image_output.close ();
			}
ulykke:
	 delete scan_line;
	 delete graph_buffer;
	 }

// i denne funktion l‘ses de ovenfor genererede billeder fra disken
// og vises p† sk‘rmen ‚t for ‚t. der anvendes en baggrunsbuffer til
// at tegne p† for at undg† interferens med sk‘rmens egen opdatering.

void animate_images () {
	 ifstream image_input;
	 int c;
	 char* graph_buffer, filename [13];

	 if ((graph_buffer = new char [block_size]) == NULL) {
			cerr << "Lagerallokeringsfejl\n";
			exit (2);
			}

	 while (c != 'q') {
			for (int picture = 0; c != 'q' && picture < pictures; picture++) {

	// †bn den aktuelle fil og v‘lg buffer for at tegne billedet

	sprintf (filename, "IMAGE%02d.3D", picture);
	image_input.open (filename, ios::in | ios::binary);
	setactivepage (1 & picture);

	// l‘s filindholdet ind og tegn det p† sk‘rmen

	for (int row = 0; c != 'q' && row < rows_per_screen; row++) {
		 image_input.read (graph_buffer, block_size);
		 putimage (0, row * row_height, graph_buffer, COPY_PUT);
		 if (kbhit ()) {
				c = getch () | 0x20;
				if (c == 'h') getch ();  // tryk p† "H" for pause...
				}
		 }
	setvisualpage (1 & picture);   // vis billedet
	image_input.close();
	}
			}
	 delete graph_buffer;
	 }

void main (int argc, char** argv) {
	 int xasp, yasp, gdriver = EGA64, gmode = EGAHI, errorcode;

	 if (argc > 1) pictures = atoi (argv [1]);

	 cout << "3D Autostereogram-animation\n"
						"(C) 1994 Maz Spork, alle rettigheder forbeholdt\n\n"
						"Dette program m† frit distribueres og ‘ndres, s† l‘nge\n"
						"denne ophavsretsbesked ikke bliver fjernet.\n\n"
						"Der bliver nu genereret " << pictures << " billeder, "
						"som lagres p† disken\n"
						"og som bagefter bliver vist i hurtig animation. Du kan trykke\n"
						"p† 'h' for at holde en pause, s† du kan finde billedet, mens\n"
						"det er roligt. Billedet er genereret til parallelsyn.\n"
						"\nGod forn›jelse. Tryk p† noget eller 'q' for at stoppe.";

	 if (getch () == 'q') exit (0);

	 registerbgidriver (EGAVGA_driver);

	 initgraph (&gdriver, &gmode, "");
	 if ((errorcode = graphresult ()) != grOk) {
			cerr << "Grafikfejl: " << grapherrormsg (errorcode) << endl;
			exit (1);
			}

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


