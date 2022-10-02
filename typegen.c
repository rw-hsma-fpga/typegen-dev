/* example1.c                                                      */
/*                                                                 */
/* This small program shows how to print a rotated string with the */
/* FreeType 2 library.                                             */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <argp.h>

#include <ft2build.h>
#include FT_FREETYPE_H

struct type_bitmap_8bit {
  int width;
  int height;
  char *buffer;
};

void make_type_bitmap(  FT_Bitmap *input_bitmap,
                        FT_Int type_width, FT_Int type_height,
                        FT_Int char_left, FT_Int char_top,
                        FT_Int char_width, FT_Int char_height,
                        struct type_bitmap_8bit *output_bitmap
                        );

void mirror_type_bitmap(  struct type_bitmap_8bit *bitmap );

void bitmap_to_textfile(struct type_bitmap_8bit *output_bitmap,
                        char* filename);

void bitmap_to_rawbitmap(struct type_bitmap_8bit *bitmap,
                        char* filename);

void bitmap_to_openscad(struct type_bitmap_8bit *bitmap,
                        char* filename);

void bitmap_to_STL(struct type_bitmap_8bit *bitmap,
                        char* filename);

const int BW_THRESHOLD = 32;


const float MM_PER_INCH = 25.4;
const float RS = 0.0285; // raster size in mm (28,5um)
const float DOD = 1.6; // Depth of drive in mm

// Vary for testing
const float TYPE_HEIGHT_IN = 0.918;
//const float TYPE_HEIGHT_IN = 0.918 + ((float)0.25/MM_PER_INCH);
//const float TYPE_HEIGHT_IN = 0.918 + ((float)0.50/MM_PER_INCH);


const float INCH_PER_PT = 0.013835; // per table; 72pt are 0.99612 inch
// const float INCH_PER_PT = 0.013888; // as 1/72th of an inch

const int ptsize = 72; // make commandline parameter


static struct argp_option options[] = {
  { "metrics",    'm', 0,         0, "output glyph metrics on stdout"},

  { "font",       'f', "FFILE",   0, "font input file"},
  // d - dpi
  // p - points
  { "dpi",        'd', "DPI",     0, "dots per inch"},
  { "character",  'c', "CHAR",    0, "input character (string format)"},

  { "textout",    't', "TBOUT",   0, "text bitmap output file path"},
  { "bitmapout",  'b', "RBOUT",   0, "raw bitmap output file path"},
  { "oscadout",   'o', "OSCFILE", 0, "OpenSCAD output file path"},
  { "stlout",     's', "STLFILE", 0, "STL output file path"},

  { NULL }
};

static char args_doc[] = ""; // TODO - function not clear yet "ARG1 ARG2"

static char doc[] = "typegen -- generating letterpress type models for 3D-printing from Truetype/OpenType data";

struct arguments
{
  // char *args[0];  0 size not allowed
  int metrics;
  char *font_file;
  char *character;
  int dpi;
  char *text_file;
  char *bitmap_file;
  char *oscad_file;
  char *stl_file;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state);

static struct argp argp = { options, parse_opt, args_doc, doc };



int main( int argc, char**  argv )
{
  FT_Library    library;
  FT_Face       face;

  FT_GlyphSlot  slot;
  FT_Error      error;


  struct arguments *arguments = calloc(1, sizeof(struct arguments));

  argp_parse (&argp, argc, argv, 0, 0, arguments);


  if ( arguments->font_file==NULL || strlen(arguments->font_file) == 0 )
  {
    fprintf ( stderr, "No font file specified.\r\n");
    exit( 1 );
  }

  if ( arguments->character==NULL || strlen(arguments->character) == 0 )
  {
    fprintf ( stderr, "No character specified.\r\n");
    exit( 1 );
  }

  error = FT_Init_FreeType( &library );              /* initialize library */
  /* error handling omitted */

  error = FT_New_Face( library, arguments->font_file, 0, &face );/* create face object */
  /* error handling omitted */

// MEASURED FOR CORRECTING
  char  MEASURED_CHAR = 'E';
  float ABOVE_CHAR_MM = 0.25;
  //;float CHAR_HEIGHT_MM_MEASURED = 20.28; // should be BODY_SIZE(ca. 25.3mm) - ABOVE_CHAR - BELOW_CHAR
  float BELOW_CHAR_MM = 4.78;

  error = FT_Set_Char_Size( face, ptsize << 6, 0, arguments->dpi, 0 );/* set char size */
  /* error handling omitted */
  slot = face->glyph;
  /* load glyph image into the slot (erase previous one) */
  error = FT_Load_Char( face, MEASURED_CHAR, FT_LOAD_RENDER );

  // metrics1
  float PX_PER_INCH = arguments->dpi;
  float INCH_PER_PX = 1/PX_PER_INCH;
  float MM_PER_PX = INCH_PER_PX * MM_PER_INCH;
    printf("MM_PER_PX: %f\r\n",MM_PER_PX);
  float body_size_inch = ptsize * INCH_PER_PT; // (body size of lead type)
  float BODY_SIZE_MM = body_size_inch * MM_PER_INCH;
  int char_width_px = slot->bitmap.width;
  int char_height_px = slot->bitmap.rows;
  float body_size_px = body_size_inch * PX_PER_INCH;


  // SCALE CORRECTION
  float CHAR_HEIGHT_MM = BODY_SIZE_MM - ABOVE_CHAR_MM - BELOW_CHAR_MM;
    printf("CHAR_HEIGHT_MM: %f\r\n",CHAR_HEIGHT_MM);
  float CHAR_HEIGHT_UNCORRECTED_MM = char_height_px * MM_PER_PX;
    printf("CHAR_HEIGHT_UNCORRECTED_MM: %f\r\n",CHAR_HEIGHT_UNCORRECTED_MM);
  int CORR_dpi = round(arguments->dpi *(CHAR_HEIGHT_MM / CHAR_HEIGHT_UNCORRECTED_MM));
    printf("CORR_dpi: %d\r\n",CORR_dpi);
  int ABOVE_CHAR_PX = (round)(ABOVE_CHAR_MM / MM_PER_PX);
    printf("ABOVE_CHAR_PX (w/ spec dpi): %d\r\n",ABOVE_CHAR_PX);
  int baseline_to_char_top_olddpi = slot->bitmap_top;
    //printf("baseline_to_char_top_olddpi (w/ spec dpi): %d\r\n",baseline_to_char_top_olddpi);
  int baseline_to_char_top_corrdpi = round(slot->bitmap_top *(CHAR_HEIGHT_MM / CHAR_HEIGHT_UNCORRECTED_MM));
    //printf("baseline_to_char_top_corrdpi (w/ corr dpi): %d\r\n",baseline_to_char_top_corrdpi);
  int CORR_TYPETOP_TO_BASELINE_PX = baseline_to_char_top_corrdpi + ABOVE_CHAR_PX;
    printf("CORR_TYPETOP_TO_BASELINE_PX: %d\r\n",CORR_TYPETOP_TO_BASELINE_PX);

  // corrected load
  error = FT_Set_Char_Size( face, ptsize << 6, 0, CORR_dpi, 0 );/* set char size */
  slot = face->glyph;
  error = FT_Load_Char( face, arguments->character[0], FT_LOAD_RENDER );
  char_width_px = slot->bitmap.width;
  char_height_px = slot->bitmap.rows;

  // metrics2
  int ascender_scaled = (int)(face->size->metrics.ascender>>6);
  int descender_scaled = (int)(face->size->metrics.descender>>6);
  int ascdesc_size_scaled = (int)((face->size->metrics.ascender)>>6) + 1 - (int)((face->size->metrics.descender)>>6);




  int advanceX_px = (int)(slot->advance.x)>>6; // round with .6 digits for accuracy? Probably fudged later anyway
  int set_width_px = advanceX_px;

  int body_size_scaled = ascdesc_size_scaled;
  float PX_PER_SCALED = body_size_px / body_size_scaled;
  float ascender_px = ascender_scaled * PX_PER_SCALED;
  float descender_px = descender_scaled * PX_PER_SCALED;

  int char_left_start = slot->bitmap_left;
  //int char_top_start = round(ascender_px - slot->bitmap_top); // old, unscaled
  int char_top_start = CORR_TYPETOP_TO_BASELINE_PX - slot->bitmap_top; // corrected stuff


  if (arguments->metrics) { //metrics
    printf("=== METRICS START ===\r\n");
    printf("face->units_per_EM  (design global [?]): %d\r\n",face->units_per_EM);
      //printf("face->bbox (design global [?]): %d\r\n",face->bbox);
      //printf("face->ascender  (design global [?]): %d\r\n",face->ascender);
      //printf("face->descender (design global [?]): %d\r\n",face->descender);
    printf("face->size->metrics.ascender   (global [scaled]): %d\r\n",ascender_scaled);
    printf("face->size->metrics.descender  (global [scaled]): %d\r\n",descender_scaled);
    printf("[Computed]  (Maximum type extent [scaled]): %d\r\n",ascdesc_size_scaled);

    printf("slot->advance.x (Advance [px]): %d\r\n",(int)(slot->advance.x)>>6);
    printf("slot->bitmap_left (Left bearing [px]): %d\r\n",slot->bitmap_left);
    printf("slot->bitmap_top  (Top bearing from baseline [px]): %d\r\n",slot->bitmap_top);

    printf("[Computed]  (Body size [scaled]): %d\r\n",body_size_scaled);
    printf("[Computed]  (Body size [in]): %f\r\n",body_size_inch);

    printf("[Computed]  (Char width [in]): %f\r\n",(float)char_width_px / PX_PER_INCH);
    printf("[Computed]  (Char height [in]): %f\r\n",(float)char_height_px / PX_PER_INCH);


    printf("----------------------\r\n");
    printf("slot->bitmap.width (Char width [px]) : %d\r\n",slot->bitmap.width);
    printf("slot->bitmap.rows  (Char height [px]): %d\r\n",slot->bitmap.rows);
    printf("[Computed]  (Body size [px]): %f\r\n",body_size_px);
    printf("[Computed]  (Set width [px]): %d\r\n",set_width_px);
    printf("[Computed]  (Ascender [px]): %f\r\n",ascender_px);
    printf("[Computed]  (Descender [px]): %f\r\n",descender_px);
    printf("[Computed]  (Char left start [px]): %d\r\n",char_left_start);
    printf("[Computed]  (Char top start [px]): %d\r\n",char_top_start);


    printf("==== METRICS END ====\r\n");
  }


  struct type_bitmap_8bit type_bm;

  make_type_bitmap( &slot->bitmap,
                    set_width_px/*based on corrected dpi, advanceX*/, body_size_px /*based on uncorrected dpi, ptsize*/,
                    char_left_start, char_top_start,
                    char_width_px, char_height_px,
                    &type_bm);

  mirror_type_bitmap(&type_bm);

  bitmap_to_textfile(&type_bm,
                     arguments->text_file);

  bitmap_to_rawbitmap(&type_bm,
                     arguments->bitmap_file);

  bitmap_to_openscad(&type_bm,
                     arguments->oscad_file);

  bitmap_to_STL(&type_bm,
                     arguments->stl_file);



  FT_Done_Face    ( face );

  FT_Done_FreeType( library );

  return 0;
}


void make_type_bitmap(  FT_Bitmap *input_bitmap,
                        FT_Int type_width, FT_Int type_height,
                        FT_Int char_left, FT_Int char_top,
                        FT_Int char_width, FT_Int char_height,
                        struct type_bitmap_8bit *output_bitmap
                        )
{
  FT_Int  x, y, x2, y2;
  unsigned char pix;
  char c;

  output_bitmap->buffer = calloc((size_t)(type_width*type_height), sizeof(unsigned char));
  output_bitmap->width = type_width;
  output_bitmap->height = type_height;
  
  unsigned char* outbuf = output_bitmap->buffer;

  for ( y = 0; y < type_height; y++ ) {
    for ( x = 0; x < type_width; x++ ) {
      *outbuf = 0x00;

      if ( (x>=char_left) && (x<char_left+char_width)
            && (y>=char_top) && (y<char_top+char_height)) {
          x2 = x - char_left;
          y2 = y - char_top;
          pix = input_bitmap->buffer[y2 * char_width + x2];
          if (pix>=BW_THRESHOLD)
            *outbuf = 0xFF;
          /*
          if (pix) {
            *outbuf = 0x30;
            while(pix) {
              *outbuf = *outbuf + 1;
              pix >>= 1;
            }
          }*/
      }
      outbuf++;
    }
  }
}


void mirror_type_bitmap(  struct type_bitmap_8bit *bitmap )
{
  FT_Int  x, y;
  int w = bitmap->width;
  int h = bitmap->height;

  unsigned char* buf = bitmap->buffer;
  unsigned char swap;

  for ( y = 0; y < h; y++ ) {
    for ( x = 0; x < (w>>1); x++ ) {
      swap = buf[y*w + x];
      buf[y*w + x] = buf[y*w + w-x];
      buf[y*w + w-x] = swap;
    }
  }
}


void bitmap_to_textfile(struct type_bitmap_8bit *bitmap,
                        char* filename)
{
  FT_Int  x, y, x2, y2;
  unsigned char pix;
  char c;


  if (!filename) {
    fprintf(stderr, "No textfile specified.\r\n");
    return;
  }

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Failed to open text bitmap file %s for writing.\r\n", filename);
    return;
  }
  unsigned char *buf = bitmap->buffer;

  for ( y = 0; y < bitmap->height; y++ ) {
    for ( x = 0; x < bitmap->width; x++ ) {
      char c = (*buf++) ? 'X' : '.';

      fputc(c, fp);fputc(c, fp);fputc(c, fp);fputc(c, fp); // TODO remove 3; just for visual scaling in Kate
    }
    fputc('\n', fp);
  }

  fclose(fp);
  fprintf(stdout, "Wrote text bitmap to %s\r\n", filename);
}


void bitmap_to_rawbitmap(struct type_bitmap_8bit *bitmap,
                        char* filename)
{
  FT_Int  x, y, x2, y2;
  unsigned char pix;
  char c;


  if (!filename) {
    fprintf(stderr, "No raw bitmap file specified.\r\n");
    return;
  }

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Failed to open raw bitmap file %s for writing.\r\n", filename);
    return;
  }
  
  fprintf(fp, "P1\r\n%d %d\r\n",bitmap->width,bitmap->height);



  unsigned char *buf = bitmap->buffer;

  for ( y = 0; y < bitmap->height; y++ ) {
    for ( x = 0; x < bitmap->width; x++ ) {
      char c = (*buf++) ? '1' : '0';
      fputc(c, fp); fputc(' ', fp);
    }
    fputc('\r', fp);fputc('\n', fp);
  }

  fclose(fp);
  fprintf(stdout, "Wrote raw PBM bitmap to %s\r\n", filename);
}


void bitmap_to_openscad(struct type_bitmap_8bit *bitmap,
                        char* filename)
{
  FT_Int  x, y, x2, y2;
  unsigned char pix;
  char c;


  if (!filename) {
    fprintf(stderr, "No OpenSCAD file specified.\r\n");
    return;
  }

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Failed to open SCAD file %s for writing.\r\n", filename);
    return;
  }

  fprintf(fp, "MM_PER_INCH = 25.4;\r\n");
  fprintf(fp, "TH = 0.918 * MM_PER_INCH; // standard type height in mm\r\n");
  fprintf(fp, "RS = 0.0285; // raster size in mm (28.5um)\r\n");
  //fprintf(fp, "DOD = RS;\r\n");
  fprintf(fp, "DOD = 0.065 * MM_PER_INCH;// depth of drive in mm\r\n");
  fprintf(fp, "BH = TH - DOD; // Body height in mm;\r\n");
  fprintf(fp, "\r\n");


  unsigned char *buf = bitmap->buffer;

  // block
  fprintf(fp, "translate([0,-RS*%d,-BH]) cube([RS*%d,RS*%d,BH]);\r\n",
          bitmap->height, bitmap->width, bitmap->height);

  // char
  fprintf(fp, "union() {\r\n");

  for ( y = 0; y < bitmap->height; y++ ) {
    for ( x = 0; x < bitmap->width; x++ ) {
      if(*buf++) {
        fprintf(fp, "translate([RS*%d,-RS*%d,0]) cube([RS,RS,DOD]);\r\n",x,y);
      }
    }
  }

  fprintf(fp, "}\r\n");

  fclose(fp);
  fprintf(stdout, "Wrote OpenSCAD code to %s\r\n", filename);
}


#define STL_triangle_write(Nx,  Ny,  Nz, \
                           v1, v2, v3) \
( { \
    TRI = (struct stl_binary_triangle) { Nx, Ny, Nz, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, 0}; \
    fwrite((void*)&TRI, 1, 50, fp); \
    tri_cnt++; \
  } )


void bitmap_to_STL(struct type_bitmap_8bit *bitmap,
                        char* filename)
{
  FT_Int  x, y;
  int i;
  int w = bitmap->width;
  int h = bitmap->height;


  if (!filename) {
    fprintf(stderr, "No STL file specified.\r\n");
    return;
  }

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Failed to open STL file %s for writing.\r\n", filename);
    return;
  }

  for(i=0;i<80;i++)
    fputc('x',fp);

  struct stl_binary_triangle {
    float Nx;  float Ny;  float Nz;
    float V1x; float V1y; float V1z;
    float V2x; float V2y; float V2z;
    float V3x; float V3y; float V3z;
    short int attr_cnt;
  };
  
  struct stl_binary_triangle TRI;

  unsigned int tri_cnt = 0;
  fwrite((void*)&tri_cnt, 1, 4, fp); // space for number of triangles

  // cube corners: upper/lower;top/botton;left/right
  typedef struct corner {
    float x, y, z;
  } corner_t;
  corner_t utl, utr, ubl, ubr, ltl, ltr, lbl, lbr;


  float BH = TYPE_HEIGHT_IN*MM_PER_INCH - DOD;// Body height in mm


  unsigned char *buf = bitmap->buffer;



  for ( y = 0; y < bitmap->height; y++ ) {
    for ( x = 0; x < bitmap->width; x++ ) {

      utl = (corner_t) { RS*x,     -RS*y, DOD };   utr = (corner_t) { RS*(x+1),     -RS*y, DOD };
      ubl = (corner_t) { RS*x, -RS*(y+1), DOD };   ubr = (corner_t) { RS*(x+1), -RS*(y+1), DOD };

      ltl = (corner_t) { RS*x,     -RS*y, 0   };   ltr = (corner_t) { RS*(x+1),     -RS*y, 0   };
      lbl = (corner_t) { RS*x, -RS*(y+1), 0   };   lbr = (corner_t) { RS*(x+1), -RS*(y+1), 0   };

      //fprintf(stdout, ".. X: %d   Y: %d\r\n", x, y);

      if ( buf[y*w+x] ) {

        //fprintf(stdout, "X: %d   Y: %d\r\n", x, y);

        // upper face
        STL_triangle_write(0,0,1,   utr, utl, ubl);
        STL_triangle_write(0,0,1,   utr, ubl, ubr);

        // left face
        if ((x==0) || (buf[ ((y)*w) + (x-1) ]==0)) {
          STL_triangle_write(-1,0,0,   ubl, utl, ltl);
          STL_triangle_write(-1,0,0,   ubl, ltl, lbl);
        }

        // right face
        if ((x==(bitmap->width-1)) || (buf[ ((y)*w) + (x+1) ]==0)) {
          STL_triangle_write(1,0,0,   ubr, ltr, utr);
          STL_triangle_write(1,0,0,   ubr, lbr, ltr);
        }

        // top face
        if ((y==0) || (buf[ ((y-1)*w) + (x) ]==0)) {
          STL_triangle_write(0,1,0,   utl, utr, ltr);
          STL_triangle_write(0,1,0,   utl, ltr, ltl);
        }

        // bottom face
        if ((y==(bitmap->height-1)) || (buf[ ((y+1)*w) + (x) ]==0)) {
          STL_triangle_write(0,-1,0,   ubl, lbr, ubr);
          STL_triangle_write(0,-1,0,   ubl, lbl, lbr);
        }

      }
      else {
                // upper faces for 0s (body)
        STL_triangle_write(0,0,1,   ltr, lbl, ltl);
        STL_triangle_write(0,0,1,   ltr, lbr, lbl);

      }
    }
  }


  utl = (corner_t) { 0,     0,   0 };   utr = (corner_t) { RS*w,     0,   0 };
  ubl = (corner_t) { 0, -RS*h,   0 };   ubr = (corner_t) { RS*w, -RS*h,   0 };

  ltl = (corner_t) { 0,     0, -BH };   ltr = (corner_t) { RS*w,     0, -BH };
  lbl = (corner_t) { 0, -RS*h, -BH };   lbr = (corner_t) { RS*w, -RS*h, -BH };

  // lower face
  STL_triangle_write(0,0,-1,   ltr, lbl, ltl);
  STL_triangle_write(0,0,-1,   ltr, lbr, lbl);

  // left face
  STL_triangle_write(-1,0,0,   ubl, utl, ltl);
  STL_triangle_write(-1,0,0,   ubl, ltl, lbl);

  // right face
  STL_triangle_write(1,0,0,   ubr, ltr, utr);
  STL_triangle_write(1,0,0,   ubr, lbr, ltr);

  // top face
  STL_triangle_write(0,1,0,   utl, utr, ltr);
  STL_triangle_write(0,1,0,   utl, ltr, ltl);

  // bottom face
  STL_triangle_write(0,-1,0,   ubl, lbr, ubr);
  STL_triangle_write(0,-1,0,   ubl, lbl, lbr);



  fprintf(stdout, "tri_cnt is %d\r\n", tri_cnt);

  fseek(fp, 80, SEEK_SET);
  fwrite((void*)&tri_cnt, 1, 4, fp);
  fclose(fp);

  fprintf(stdout, "Wrote binary STL data to %s\r\n", filename);
}

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'm':
      arguments->metrics = 1;
      break;
    case 'f':
      arguments->font_file = arg;
      break;
    case 'c':
      arguments->character = arg;
      break;
    case 'd':
      arguments->dpi = atoi(arg);
      break;
    case 't':
      arguments->text_file = arg;
      break;
    case 'b':
      arguments->bitmap_file = arg;
      break;
    case 'o':
      arguments->oscad_file = arg;
      break;
    case 's':
      arguments->stl_file = arg;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 0)
        /* Too many arguments. */
        argp_usage (state);

      // arguments->args[state->arg_num] = arg; 0 size not allowed

      break;

    case ARGP_KEY_END:
      if (state->arg_num < 0)
        /* Not enough arguments. */
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/*
  // cube with sizes x=1,y=2,z=3
  utl = (corner_t) { 0,  0, 3 };  utr = (corner_t) { 1,  0, 3 };
  ubl = (corner_t) { 0, -2, 3 };  ubr = (corner_t) { 1, -2, 3 };

  ltl = (corner_t) { 0,  0, 0 };  ltr = (corner_t) { 1,  0, 0 };
  lbl = (corner_t) { 0, -2, 0 };  lbr = (corner_t) { 1, -2, 0 };

//  STL_triangle_write(n,n,n,   v,v,v);

  // upper face
  STL_triangle_write(0,0,1,   utr, utl, ubl);
  STL_triangle_write(0,0,1,   utr, ubl, ubr);

  // lower face
  STL_triangle_write(0,0,-1,   ltr, lbl, ltl);
  STL_triangle_write(0,0,-1,   ltr, lbr, lbl);

  // left face
  STL_triangle_write(-1,0,0,   ubl, utl, ltl);
  STL_triangle_write(-1,0,0,   ubl, ltl, lbl);

  // right face
  STL_triangle_write(1,0,0,   ubr, ltr, utr);
  STL_triangle_write(1,0,0,   ubr, lbr, ltr);

  // top face
  STL_triangle_write(0,1,0,   utl, utr, ltr);
  STL_triangle_write(0,1,0,   utl, ltr, ltl);

  // bottom face
  STL_triangle_write(0,-1,0,   ubl, lbr, ubr);
  STL_triangle_write(0,-1,0,   ubl, lbl, lbr);
  */