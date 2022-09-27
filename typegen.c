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


void draw_to_textfile(  FT_Bitmap*  bitmap,
                        FT_Int type_width, FT_Int type_height,
                        FT_Int char_left, FT_Int char_top,
                        FT_Int char_width, FT_Int char_height,
                        char* filename);

const double INCH_PER_PT = 0.013835; // per table; 72pt are 0.99612 inch
// const double INCH_PER_PT = 0.013888; // as 1/72th of an inch

const int dpi = 891; // make commandline parameter
const int ptsize = 72; // make commandline parameter


static struct argp_option options[] = {
  { "metrics",    'm', 0,         0, "output glyph metrics on stdout"},

  { "font",       'f', "FFILE",   0, "font input file"},
  // d - dpi
  // p - points
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

  /* use 72pt at 891dpi */
  error = FT_Set_Char_Size( face, 72 << 6, 0, 891, 0 );/* set char size */
  /* error handling omitted */

  /* cmap selection omitted;                                        */
  /* for simplicity we assume that the font contains a Unicode cmap */

  slot = face->glyph;

  /* load glyph image into the slot (erase previous one) */
  error = FT_Load_Char( face, arguments->character[0], FT_LOAD_RENDER );

  // metrics
  int ascender_scaled = (int)(face->size->metrics.ascender>>6);
  int descender_scaled = (int)(face->size->metrics.descender>>6);
  int ascdesc_size_scaled = (int)((face->size->metrics.ascender)>>6) + 1 - (int)((face->size->metrics.descender)>>6);


  double PX_PER_INCH = dpi;
  double INCH_PER_PX = 1/PX_PER_INCH;

  int char_width_px = slot->bitmap.width;
  int char_height_px = slot->bitmap.rows;
  int advanceX_px = (int)(slot->advance.x)>>6;
  int set_width_px = advanceX_px;

  int body_size_scaled = ascdesc_size_scaled;
  double body_size_inch = ptsize * INCH_PER_PT; //(body size of lead type)
  double body_size_px = body_size_inch * PX_PER_INCH;
  double PX_PER_SCALED = body_size_px / body_size_scaled;
  double ascender_px = ascender_scaled * PX_PER_SCALED;
  double descender_px = descender_scaled * PX_PER_SCALED;

  int char_left_start = slot->bitmap_left;
  int char_top_start = round(ascender_px - slot->bitmap_top);



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

  draw_to_textfile( &slot->bitmap,
                    set_width_px, body_size_px,
                    char_left_start, char_top_start,
                    char_width_px, char_height_px,
                    arguments->text_file);
//  draw_to_textfile( &slot->bitmap, slot->bitmap_left, target_height - slot->bitmap_top, arguments->text_file);

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

  return 0;
}

void draw_to_textfile(  FT_Bitmap*  bitmap,
                        FT_Int type_width, FT_Int type_height,
                        FT_Int char_left, FT_Int char_top,
                        FT_Int char_width, FT_Int char_height,
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

  /* for simplicity, we assume that `bitmap->pixel_mode' */
  /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

  for ( y = 0; y < type_height; y++ ) {
    for ( x = 0; x < type_width; x++ ) {
      c = '.';

      if ( (x>=char_left) && (x<char_left+char_width)
            && (y>=char_top) && (y<char_top+char_height)) {
          x2 = x - char_left;
          y2 = y - char_top;
          pix = bitmap->buffer[y2 * char_width + x2];
          
          //c = (pix == 0) ? '.' : ((pix < 32) ? '+' : 'X') ;
          if (pix) {
            c = '0'; 
            while(pix) {
              c++;
              pix >>= 1;
            }
          }
      }

      fputc(c, fp);fputc(c, fp);fputc(c, fp);fputc(c, fp); // TODO remove 3; just for visual scaling in Kate
    }
    fputc('\n', fp);
  }

  fclose(fp);
  fprintf(stdout, "Wrote text bitmap to %s\r\n", filename);
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

