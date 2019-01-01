#include <graphics.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

int images_per_screen, maxx, maxy, pixels_per_image, maxcolour;

/* this function returns the height of the pixel in the source picture
 * at location (x,y)
 * ...for now it just creates a stash of boxes with increasing heights
*/

int height (int x, int y) {
    const N = 32;   /* how high do I want the ``pyramid''? */

    for (int n = 1; n<=N; n++)
        if (x > (maxx/2 - (maxx/N)*n) && x < (maxx/2 + (maxx/N)*n)
        &&  y > (maxy/2 - (maxy/N)*n) && y < (maxy/2 + (maxy/N)*n))
            return N-n;
    return 0;
    }

void create_basic_pattern () {
   int x, y, xx=0;  /* xx is the pattern band to use for init data */

   for (y = 0; y < maxy; y++)
       for (x = 0; x < pixels_per_image; x++)
           if (rand () & 1)  /* should we plot a pixel here? */
                  putpixel (xx * pixels_per_image + x, y, random (maxcolour+1));
}

void create_circles_with_crosses () {
   for (int y = 0; y < maxy; y+=pixels_per_image) {
       for (int r = 0; r < pixels_per_image / 2; r+= 4)
          circle (pixels_per_image / 2, y + pixels_per_image / 2, r);

//       line (0, y, pixels_per_image - 1, y + pixels_per_image - 1);
//       line (pixels_per_image - 1, y, 0, y + pixels_per_image - 1);
       }
   }

int main(void)
{
   int gdriver = DETECT, gmode, errorcode;
   initgraph (&gdriver, &gmode, "");  /* driver must be in current dir */
   errorcode = graphresult();
   if (errorcode != grOk) {
      printf("Graphics error: %s\n", grapherrormsg (errorcode));
      exit(1);
      }

   images_per_screen = 10;
   maxx = getmaxx ();
   maxy = getmaxy ();
   pixels_per_image = maxx / images_per_screen;
   maxcolour = getmaxcolor ();

   /* create the stereoscopic pattern */

   create_basic_pattern ();
//   create_circles_with_crosses ();

   /* create the superimposition */

   for (int y = 0; y < maxy; y++)
      for (int xx = 1; xx < images_per_screen; xx++)
         for (int x = 0; x < pixels_per_image; x++) {
             int dst = xx * pixels_per_image + x;
             int src = dst - pixels_per_image;
             putpixel (dst, y, getpixel (src + height (src, y), y));
             }
/*
            putpixel (xx * pixels_per_image + x, y,
                     getpixel ((xx-1) * pixels_per_image + x +
                        height (xx * pixels_per_image + x, y), y));
*/

   getch ();
   closegraph ();
   return 0;
}

