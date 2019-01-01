#include <iostream.h>
#include <math.h>

void main () {
   double n = 0.25 * 3.1415926 * 2;
   double d = 0;

   cout << "d\tcosd\tcos(d+n)\n";

   while (d < 15) {
	cout << d << "\t" << cos (d) << "\t" << cos (d + n) << endl;
	d++;
	}

   }