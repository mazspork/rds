// decode.cpp
// LZW decoding of GIF images

#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>

extern int colours;
static istream* current_file;

short get_byte () {
   unsigned char ch;
   current_file->get (ch);
   if (current_file->eof ()) {
      cout << "unexpected end-of-file\n";
      return -1;
      }
   return ch;
   }

typedef void (*ProcessRasterData) (char*, int);

static int bad_code_count;
static unsigned char buffer [7], packed_fields, color_index, aspect;
static unsigned char separator, red, green, blue;
unsigned int l_width, l_height, left, top, p_width, p_height;

static short curr_size;                 // The current code size
static short clear;                     // Value for a clear code
static short ending;                    // Value for a ending code
static short newcodes;                  // First available code
static short top_slot;                  // Highest code for current size
static short slot;                      // Last read code
static short navail_bytes = 0;          // # bytes left in block
static short nbits_left = 0;            // # bits left in current byte
static unsigned char b1;                // Current byte
static unsigned char byte_buff[257];    // Current block
static unsigned char *pbytes;           // Pointer to next byte in block

static long code_mask [13] = {
   0,
   0x0001, 0x0003,
   0x0007, 0x000F,
   0x001F, 0x003F,
   0x007F, 0x00FF,
   0x01FF, 0x03FF,
   0x07FF, 0x0FFF
   };

// get_next_code() - gets next code from GIF file and returns this code
// or some error

static short get_next_code () {
   short i, x;
   unsigned long ret;

   if (!nbits_left) {
      if (navail_bytes <= 0) {   // out of bytes, read next block
         pbytes = byte_buff;
         if ((navail_bytes = get_byte ()) < 0)
            return navail_bytes;
         else
            if (navail_bytes)
               for (i = 0; i < navail_bytes; i++) {
                  if ((x = get_byte()) < 0) return x;
                  byte_buff [i] = x;
                  }
         }
      b1 = *pbytes++, nbits_left = 8, navail_bytes--;
      }

   ret = b1 >> (8 - nbits_left);
   while (curr_size > nbits_left) {
      if (navail_bytes <= 0) {        // out of bytes, read next block
         pbytes = byte_buff;
         if ((navail_bytes = get_byte()) < 0) return navail_bytes;
         else if (navail_bytes) {
            for (i = 0; i < navail_bytes; i++) {
               if ((x = get_byte()) < 0) return x;
               byte_buff[i] = x;
               }
            }
         }
      b1 = *pbytes++, ret |= b1<<nbits_left, nbits_left+=8, navail_bytes--;
      }
   nbits_left -= curr_size, ret &= code_mask [curr_size];
   return short (ret);
   }

const MAX_CODES = 4095;

static unsigned char stack [MAX_CODES + 1];   /* Stack for storing pixels */
static unsigned char suffix [MAX_CODES + 1];  /* Suffix table */
static unsigned short prefix [MAX_CODES + 1]; /* Prefix linked list */

// get a 16-byte little-endian from input stream

unsigned short getword (istream& is) {
   unsigned char lsb, msb;
   is.get (lsb).get (msb);
   return lsb + 0x100 * msb;
   }

unsigned char *sp, *bufptr;
char *buf;
short code, fc, oc, bufcnt;
short c, size, linewidth;

// get image header

void read_gif_header (istream& input_file) {

   current_file = &input_file;

   for (int i = 0; i < 6; i++) current_file->get (buffer [i]);
   l_height = getword (*current_file);
   l_width = getword (*current_file);
   current_file->get (packed_fields).get (color_index).get (aspect);
   if (packed_fields & 0x80) {   // read color table
      colours = 1 << ((packed_fields & 7) + 1);
      for (i = 0; i < colours; i++)
         (*current_file).get (red).get (green).get (blue);
      }

   current_file->get (separator);

   left = getword (*current_file);
   top = getword (*current_file);

   p_width = getword (*current_file);
   p_height = getword (*current_file);

   current_file->get (packed_fields);

   if (packed_fields & 0x80)    // read color table
      for (int i = 0, gct_size = 1 << ((packed_fields & 7) + 1);
         i < gct_size; i++) (*current_file).get (red).get (green).get (blue);

   linewidth = p_width;

   if ((size = get_byte ()) < 0) return;
   if (2 > size || size > 9) return;

   // initialisations...

   curr_size = size + 1, top_slot = 1 << curr_size, clear = 1 << size,
   ending = clear + 1, slot = newcodes = ending + 1,
   navail_bytes = nbits_left = 0, oc = fc = 0,
   buf = new char [linewidth + 1], sp = stack, bufptr = buf,
   bufcnt = linewidth;
   }

// short gif_decoder : decodes an LZW image according to the "GIF89a"
// specifications. It's ugly and bulky - but it does the job.
// calls through "PassedFunction" for every decoded raster line

short gif_decode (ProcessRasterData PassedFunction) {
   while ((c = get_next_code ()) != ending) {
      if (c < 0) {
         delete buf;
         return 0;
         }
      if (c == clear) {
         curr_size = size + 1, slot = newcodes, top_slot = 1 << curr_size;
         while ((c = get_next_code ()) == clear) /*  drain clear codes */ ;
         if (c == ending) break;
         if (c >= slot) c = 0;
         oc = fc = c, *bufptr++ = c;
         if (!--bufcnt) {
            (*PassedFunction) (buf, linewidth);
            bufptr = buf, bufcnt = linewidth;
            }
         }
      else {
         code = c;
         if (code >= slot) {
            if (code > slot) bad_code_count++;
            code = oc, *sp++ = fc;
            }

         while (code >= newcodes)
            *sp++ = suffix [code], code = prefix [code];

         *sp++ = code;

         if (slot < top_slot)
            suffix [slot] = fc = code, prefix [slot++] = oc, oc = c;

         if (slot >= top_slot && curr_size < 12)
            top_slot <<= 1, curr_size++;

         while (sp > stack) {
            *bufptr++ = *--sp;
            if (!--bufcnt) {
               (*PassedFunction) (buf, linewidth);
               bufptr = buf, bufcnt = linewidth;
               }
            }
         }
      }

   if (bufcnt != linewidth) (*PassedFunction) (buf, linewidth - bufcnt); // kludge
   delete buf;
   return 0;
   }

