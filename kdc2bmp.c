/*
 * kdc2bmp.c - Converts an uncompressed DC-120 .kdc file to a .bmp file
 *
 * The 848x976 CCD data is converted to 1272x976.
 *
 * The CCD data starts at offset 15680 in the .kdc file, and
 * is stored one row at a time.  Each row is rotated by a
 * pseudo-random number (in kdcoff below).
 *
 * Once the data is unrotated it is laid out in a Bayer pattern:
 *
 *   GRGRGR (R is red, G is green, B is blue)
 *   BGBGBG
 *   GRGRGR
 *   BGBGBG
 *
 * The RGB data is then linearly interpolated, and then stretched
 * by a factor of 1.5 in the horizontal direction.  This results
 * in a 1272x976 image, which has an aspect ratio of 1.3.
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

/*
 * Data for bmp header (24 bits per pixel, 1272x976)
 */

static unsigned char bmphead[] = {
  0x42, 0x4D, 0x36, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0xF8, 0x04, 0x00, 0x00, 0xD0, 0x03,
  0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*
 * Gamma correction table for gamma = 1.7
 */

static unsigned char gamma[] = {
    0, 10, 15, 19, 22, 25, 28, 31, 33, 36, 38, 40, 42, 44, 46, 48,
   50, 52, 54, 55, 57, 59, 60, 62, 64, 65, 67, 68, 70, 71, 72, 74,
   75, 77, 78, 79, 81, 82, 83, 84, 86, 87, 88, 89, 91, 92, 93, 94,
   95, 97, 98, 99,100,101,102,103,105,106,107,108,109,110,111,112,
  113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
  129,130,131,132,133,134,135,135,136,137,138,139,140,141,142,143,
  144,144,145,146,147,148,149,150,150,151,152,153,154,155,156,156,
  157,158,159,160,160,161,162,163,164,164,165,166,167,168,168,169,
  170,171,172,172,173,174,175,175,176,177,178,178,179,180,181,181,
  182,183,184,184,185,186,187,187,188,189,190,190,191,192,192,193,
  194,195,195,196,197,197,198,199,199,200,201,202,202,203,204,204,
  205,206,206,207,208,208,209,210,210,211,212,212,213,214,214,215,
  216,216,217,218,218,219,220,220,221,222,222,223,224,224,225,226,
  226,227,227,228,229,229,230,231,231,232,233,233,234,234,235,236,
  236,237,238,238,239,239,240,241,241,242,242,243,244,244,245,245,
  246,247,247,248,248,249,250,250,251,251,252,253,253,254,254,255
};

/*
 * Data for decoding uncompressed .kdc files.  Kodak has gone
 * to a great deal of trouble to make it hard to reverse-engineer
 * this table, but I was able to compute it by calculating the
 * offset at which scan lines were correlated.  Ed Hamrick, 10/24/97
 */

static short int kdcoff[] = {
    0,828,798,488,648,748,698,  8,448,668,598,376,248,588,498,744,
   48,508,398,264,696,428,298,632,496,348,198,152,296,268, 98,520,
   96,188,846, 40,744,108,746,408,544, 28,646,776,344,796,546,296,
  144,716,446,664,792,636,346,184,592,556,246,552,392,476,146, 72,
  192,396, 46,440,840,316,794,808,640,236,694,328,440,156,594,696,
  240, 76,494,216, 40,844,394,584,688,764,294,104,488,684,194,472,
  288,604, 94,840, 88,524,842,360,736,444,742,728,536,364,642,248,
  336,284,542,616,136,204,442,136,784,124,342,504,584, 44,242, 24,
  384,812,142,392,184,732, 42,760,832,652,790,280,632,572,690,648,
  432,492,590,168,232,412,490,536, 32,332,390, 56,680,252,290,424,
  480,172,190,792,280, 92, 90,312, 80, 12,838,680,728,780,738,200,
  528,700,638,568,328,620,538, 88,128,540,438,456,776,460,338,824,
  576,380,238,344,376,300,138,712,176,220, 38,232,824,140,786,600,
  624, 60,686,120,424,828,586,488,224,748,486,  8, 24,668,386,376,
  672,588,286,744,472,508,186,264,272,428, 86,632, 72,348,834,152,
  720,268,734,520,520,188,634, 40,320,108,534,408,120, 28,434,776,
  768,796,334,296,568,716,234,664,368,636,134,184,168,556, 34,552,
  816,476,782, 72,616,396,682,440,416,316,582,808,216,236,482,328,
   16,156,382,696,664, 76,282,216,464,844,182,584,264,764, 82,104,
   64,684,830,472,712,604,730,840,512,524,630,360,312,444,530,728,
  112,364,430,248,760,284,330,616,560,204,230,136,360,124,130,504,
  160, 44, 30, 24,808,812,778,392,608,732,678,760,408,652,578,280,
  208,572,478,648,  8,492,378,168,656,412,278,536,456,332,178, 56,
  256,252, 78,424, 56,172,826,792,704, 92,726,312,504, 12,626,680,
  304,780,526,200,104,700,426,568,752,620,326, 88,552,540,226,456,
  352,460,126,824,152,380, 26,344,800,300,774,712,600,220,674,232,
  400,140,574,600,200, 60,474,120,  0,828,374,488,648,748,274,  8,
  448,668,174,376,248,588, 74,744, 48,508,822,264,696,428,722,632,
  496,348,622,152,296,268,522,520, 96,188,422, 40,744,108,322,408,
  544, 28,222,776,344,796,122,296,144,716, 22,664,792,636,770,184,
  592,556,670,552,392,476,570, 72,192,396,470,440,840,316,370,808,
  640,236,270,328,440,156,170,696,240, 76, 70,216, 40,844,818,584,
  688,764,718,104,488,684,618,472,288,604,518,840, 88,524,418,360,
  736,444,318,728,536,364,218,248,336,284,118,616,136,204, 18,136,
  784,124,766,504,584, 44,666, 24,384,812,566,392,184,732,466,760,
  832,652,366,280,632,572,266,648,432,492,166,168,232,412, 66,536,
   32,332,814, 56,680,252,714,424,480,172,614,792,280, 92,514,312,
   80, 12,414,680,728,780,314,200,528,700,214,568,328,620,114, 88,
  128,540, 14,456,776,460,762,824,576,380,662,344,376,300,562,712,
  176,220,462,232,824,140,362,600,624, 60,262,120,424,828,162,488,
  224,748, 62,  8, 24,668,810,376,672,588,710,744,472,508,610,264,
  272,428,510,632, 72,348,410,152,720,268,310,520,520,188,210, 40,
  320,108,110,408,120, 28, 10,776,768,796,758,296,568,716,658,664,
  368,636,558,184,168,556,458,552,816,476,358, 72,616,396,258,440,
  416,316,158,808,216,236, 58,328, 16,156,806,696,664, 76,706,216,
  464,844,606,584,264,764,506,104, 64,684,406,472,712,604,306,840,
  512,524,206,360,312,444,106,728,112,364,  6,248,760,284,754,616,
  560,204,654,136,360,124,554,504,160, 44,454, 24,808,812,354,392,
  608,732,254,760,408,652,154,280,208,572, 54,648,  8,492,802,168,
  656,412,702,536,456,332,602, 56,256,252,502,424, 56,172,402,792,
  704, 92,302,312,504, 12,202,680,304,780,102,200,104,700,  2,568,
  752,620,750, 88,552,540,650,456,352,460,550,824,152,380,450,344,
  800,300,350,712,600,220,250,232,400,140,150,600,200, 60, 50,120,
    0,828,798,488,648,748,698,  8,448,668,598,376,248,588,498,744,
   48,508,398,264,696,428,298,632,496,348,198,152,296,268, 98,520,
   96,188,846, 40,744,108,746,408,544, 28,646,776,344,796,546,296,
  144,716,446,664,792,636,346,184,592,556,246,552,392,476,146, 72,
  192,396, 46,440,840,316,794,808,640,236,694,328,440,156,594,696,
  240, 76,494,216, 40,844,394,584,688,764,294,104,488,684,194,472,
  288,604, 94,840, 88,524,842,360,736,444,742,728,536,364,642,248,
  336,284,542,616,136,204,442,136,784,124,342,504,584, 44,242, 24,
};

#define CCDWID 848
#define CCDHEI 976
#define CCDOFF 15680

/* Storage for the CCD data */
unsigned char ccd[CCDHEI][CCDWID];

/* Storage for red, green, blue data, before expanding 1.5x */
unsigned char red[CCDHEI][CCDWID];
unsigned char gre[CCDHEI][CCDWID];
unsigned char blu[CCDHEI][CCDWID];

unsigned char hdr[CCDOFF];

void main(int argc, char **argv)
{
  FILE *kdc;
  FILE *bmp;

  char name[512];

  int  j, k;

  fprintf(stdout, "\n"
                  "KDC2BMP - convert Kodak's DC120 .KDC file format to .BMP\n"
                  "Based on code by Ed Hamrick, http://www.hamrick.com\n"
                  "OS/2 version compiled by St‚phane Charette, charette@writeme.com\n\n" );

  /* Make sure called with only one argument */
  if (argc != 2)
  {
    fprintf(stderr, "Usage: kdc2bmp.exe filename\n\n"
                     "Note that you must not specify the file extension,\n"
                     "and that only uncompressed .KDC files are supported.\n");
    return;
  }

  /* Open the .kdc file */
  strcpy(name, argv[1]);
  strcat(name, ".kdc");
  kdc = fopen(name, "rb");
  fprintf(stdout, "...reading %s...\n", name );
  if (!kdc)
  {
    fprintf(stderr,"Error: can't open %s\n",name);
    return;
  }

  /* Verify that it's an uncompressed .kdc file */
  fread(hdr, 1, sizeof(hdr), kdc);

  if (strcmp(hdr+470, "Kodak DC120 ZOOM Digital Camera"))
  {
    fprintf(stderr, "Error: not a DC120 .kdc file\n");
    return;
  }

  if (hdr[707] != 1)
  {
    fprintf(stderr, "Error: not an uncompressed .kdc file\n");
    return;
  }

  /* Read in and rotate the uncompressed data */
  for (k=0; k<CCDHEI; k++)
  {
    for (j=0; j<CCDWID; j++)
    {
      ccd[k][(j-kdcoff[k]+CCDWID) % CCDWID] = fgetc(kdc);
    }
  }

  /* Close the .kdc file */
  fclose(kdc);

  /* Use pixel replication to start */
  for (k=0; k<CCDHEI; k+=2)
  {
    for (j=0; j<CCDWID; j+=2)
    {
      red[k  ][j  ] = ccd[k  ][j+1];
      red[k  ][j+1] = ccd[k  ][j+1];
      red[k+1][j  ] = ccd[k  ][j+1];
      red[k+1][j+1] = ccd[k  ][j+1];

      gre[k  ][j  ] = ccd[k  ][j  ];
      gre[k  ][j+1] = ccd[k  ][j  ];
      gre[k+1][j  ] = ccd[k+1][j+1];
      gre[k+1][j+1] = ccd[k+1][j+1];

      blu[k  ][j  ] = ccd[k+1][j  ];
      blu[k  ][j+1] = ccd[k+1][j  ];
      blu[k+1][j  ] = ccd[k+1][j  ];
      blu[k+1][j+1] = ccd[k+1][j  ];
    }
  }

  /* Compute the interpolated red data */
  for (k=2; k<CCDHEI-2; k+=2)
  {
    for (j=3; j<CCDWID-2; j+=2)
    {
      /* Get the left, bottom, corner red pixels */
      unsigned char r, rl, rb, rc;

      r  = ccd[k  ][j  ];
      rl = ccd[k  ][j-2];
      rb = ccd[k+2][j  ];
      rc = ccd[k+2][j-2];

      red[k  ][j-1] = (r + rl          ) / 2;
      red[k+1][j  ] = (r      + rb     ) / 2;
      red[k+1][j-1] = (r + rl + rb + rc) / 4;
    }
  }

  /* Compute the interpolated green data */
  for (k=2; k<CCDHEI-2; k+=2)
  {
    for (j=3; j<CCDWID-2; j+=2)
    {
      /* Get the left, right, top, bottom green pixels */
      unsigned char gl, gr, gt, gb;

      gl = ccd[k  ][j-1];
      gr = ccd[k  ][j+1];
      gt = ccd[k-1][j  ];
      gb = ccd[k+1][j  ];

      gre[k][j] = (gl + gr + gt + gb) / 4;
    }
  }

  for (k=3; k<CCDHEI-2; k+=2)
  {
    for (j=2; j<CCDWID-2; j+=2)
    {
      /* Get the left, right, top, bottom green pixels */
      unsigned char gl, gr, gt, gb;

      gl = ccd[k  ][j-1];
      gr = ccd[k  ][j+1];
      gt = ccd[k-1][j  ];
      gb = ccd[k+1][j  ];

      gre[k][j] = (gl + gr + gt + gb) / 4;
    }
  }

  /* Compute the interpolated blue data */
  for (k=3; k<CCDHEI-2; k+=2)
  {
    for (j=2; j<CCDWID-2; j+=2)
    {
      /* Get the right, top, corner blue pixels */
      unsigned char b, br, bt, bc;

      b  = ccd[k  ][j  ];
      br = ccd[k  ][j+2];
      bt = ccd[k-2][j  ];
      bc = ccd[k-2][j+2];

      blu[k  ][j+1] = (b + br          ) / 2;
      blu[k-1][j  ] = (b      + bt     ) / 2;
      blu[k-1][j+1] = (b + br + bt + bc) / 4;
    }
  }

  /* Open the .bmp file */
  strcpy(name, argv[1]);
  strcat(name, ".bmp");
  fprintf(stdout, "...writing %s...\n", name );
  bmp = fopen(name, "wb");
  if (!bmp)
  {
    fprintf(stderr,"Error: can't create %s\n",name);
    return;
  }

  /* Write out the .bmp header */
  fwrite(bmphead, 1, sizeof(bmphead), bmp);

  /* Write out the gamma corrected, stretched pixel data */
  for (k=CCDHEI-1; k>=0; k--)
  {
    for (j=0; j<CCDWID; j+=2)
    {
      fputc(gamma[(blu[k][j]            )  ], bmp);
      fputc(gamma[(gre[k][j]            )  ], bmp);
      fputc(gamma[(red[k][j]            )  ], bmp);

      fputc(gamma[(blu[k][j]+blu[k][j+1])/2], bmp);
      fputc(gamma[(gre[k][j]+gre[k][j+1])/2], bmp);
      fputc(gamma[(red[k][j]+red[k][j+1])/2], bmp);

      fputc(gamma[(          blu[k][j+1])  ], bmp);
      fputc(gamma[(          gre[k][j+1])  ], bmp);
      fputc(gamma[(          red[k][j+1])  ], bmp);
    }
  }

  /* Close the .bmp file */
  fclose(bmp);
}

