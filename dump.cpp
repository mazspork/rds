#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>

void main (int argc, char** argv) {
   if (argc < 2) { cout << "usage: dump <file>\n"; exit (1); }
   cout.precision (2);
   cout.fill ('0');
   cout << hex;
   for (int i = 1; i < argc; i++) {
      ifstream input (argv [i], ios::binary);
      unsigned char ch, j = 0;
      unsigned char v [17];
      cout << "File: " << argv [i] << "\n\n";
      while (input) {
         input.get (ch);
         if (ch < 16) cout << "0";
         cout << unsigned (ch) << " ";
         v [j++] = (ch > 31 && ch < 127) ? ch : '.', v [j] = 0;
         if (j == 16) {
            j = 0;
            cout << ": " << v << endl;
            }
         }
      if (j) {
         for (int i = 0; i < 16 - j; i++) cout << "   ";
         cout << ": " << v;
         }
      cout << endl << endl;
      }
   }