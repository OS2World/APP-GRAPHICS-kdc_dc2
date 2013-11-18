/*
 * kdc2jpg.c - Converts a compressed DC-120 .kdc file to a .jpg file
 *
 * The data in the .kdc file is actually just a JPEG file starting
 * at offset 15680.  The JPEG data is stored as an 848x488 image.
 * with 100 dots per inch horizontally and 75 dots per inch
 * vertically.  This utility just strips off the .KDC header and
 * adds a JPEG JFIF header.
 *
 * The CCD data that was used to create the JPEG data is an array
 * of 848x976 pixels, organized as:
 *
 *   GRGRGR (R is red, G is green, B is blue)
 *   BGBGBG
 *   GRGRGR
 *   BGBGBG
 *
 * The camera combines each pair of CCD rows and then JPEG
 * compresses it.  To accurately reconstruct the image from the
 * JPEG data, the 488 lines need to be split apart into 976
 * rows in the original CCD Bayer pattern.  Then a linear
 * interpolation needs to be applied to construct a full-resolution
 * image (along with a factor of 1.5 stretching horizontally).
 *
 * Author  : Ed Hamrick
 * Date    : October 24, 1997
 * See also: http://www.hamrick.com/
 */

/*
 * Further modifications by Stephane Charette, charette@writeme.com
 * 1998Feb01
 * compiled for OS/2 and uploaded to the internet as freeware, SC, 1998Feb01
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char jpghead[] = {
  0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46,
  0x49, 0x46, 0x00, 0x01, 0x02, 0x01, 0x00, 0x64,
  0x00, 0x4B, 0x00, 0x00
};

#define CCDOFF 15680

unsigned char hdr[CCDOFF];

void main(int argc, char *argv[])
{
  FILE *kdc;
  FILE *jpg;

  char name[512];

  fprintf(stdout, "\n"
                  "KDC2JPG - convert Kodak's DC120 .KDC file format to .JPG\n"
                  "Based on code by Ed Hamrick, http://www.hamrick.com\n"
                  "OS/2 version compiled by St‚phane Charette, charette@writeme.com\n\n" );

  /* Make sure called with only one argument */
  if (argc != 2)
  {
    fprintf(stdout,  "Usage: kdc2jpg.exe filename\n\n"
                     "Note that you must not specify the file extension,\n"
                     "and that only compressed .KDC files are supported.\n");
    return;
  }

  /* Open the .kdc file */
  strcpy(name, argv[1]);
  strcat(name, ".kdc");
  fprintf(stdout, "...reading %s...\n", name );
  kdc = fopen(name,"rb");
  if (!kdc)
  {
    fprintf(stderr,"Error: can't open %s\n",name);
    return;
  }

  /* Verify that it's a compressed .kdc file */
  fread(hdr, 1, sizeof(hdr), kdc);

  if (strcmp(hdr+470, "Kodak DC120 ZOOM Digital Camera"))
  {
    fprintf(stderr, "Error: not a DC120 .kdc file\n");
    return;
  }

  if (hdr[707] != 7)
  {
    fprintf(stderr, "Error: not a compressed .kdc file\n");
    return;
  }

  /* Open the .jpg file */
  strcpy(name, argv[1]);
  strcat(name, ".jpg");
  fprintf(stdout, "...writing %s...\n", name );
  jpg = fopen(name, "wb");
  if (!jpg)
  {
    fprintf(stderr,"Error: can't create %s\n",name);
    return;
  }

  /* Write out the .jpg header */
  fwrite(jpghead,1,sizeof(jpghead),jpg);

  /* Discard the extra marker  (included already in jpghead) */
  fgetc(kdc);
  fgetc(kdc);

  /* Read data in pairs and exit if eof */
  while(1)
  {
    int c1, c2;

    c1 = fgetc(kdc);
    c2 = fgetc(kdc);

    if ((c1 < 0) || (c2 < 0))
      break;

    fputc(c2, jpg);
    fputc(c1, jpg);
  }

  /* Close the .jpg and .kdc files */
  fclose(jpg);
  fclose(kdc);
}

