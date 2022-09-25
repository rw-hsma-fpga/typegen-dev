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


#define WIDTH   1280
#define HEIGHT  1000


/* origin is the upper left corner */
unsigned char image[HEIGHT][WIDTH];


void draw_to_textfile( FT_Bitmap*  bitmap,FT_Int x, FT_Int y, char* filename);
void draw_bitmap( FT_Bitmap*  bitmap,FT_Int x, FT_Int y);
void show_image( void );




static struct argp_option options[] = {
  { "metrics",    'm', 0,         0, "output glyph metrics on stdout"},

  { "font",       'f', "FFILE",   0, "font input file"},
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

  char*         filename;
  char*         text;

  int           target_height;

  struct arguments *arguments = calloc(1, sizeof(struct arguments));

  argp_parse (&argp, argc, argv, 0, 0, arguments);
/*
  if ( argc != 3 ) {
    fprintf ( stderr, "usage: %s font sample-text\n", argv[0] );
    exit( 1 );
  }

  filename      = argv[1];                    
  text          = argv[2];                     
*/

  filename = arguments->font_file;
  text = arguments->character;

  if ( filename==NULL || strlen(filename) == 0 )
  {
    fprintf ( stderr, "No font file specified.\r\n");
    exit( 1 );
  }

  if ( text==NULL || strlen(text) == 0 )
  {
    fprintf ( stderr, "No character specified.\r\n");
    exit( 1 );
  }

  target_height = HEIGHT;

  error = FT_Init_FreeType( &library );              /* initialize library */
  /* error handling omitted */

  error = FT_New_Face( library, filename, 0, &face );/* create face object */
  /* error handling omitted */

  /* use 72pt at 891dpi */
  error = FT_Set_Char_Size( face, 72 << 6, 0, 891, 0 );/* set char size */
  /* error handling omitted */

  /* cmap selection omitted;                                        */
  /* for simplicity we assume that the font contains a Unicode cmap */

  slot = face->glyph;

  /* load glyph image into the slot (erase previous one) */
  error = FT_Load_Char( face, text[0], FT_LOAD_RENDER );

  draw_to_textfile( &slot->bitmap, slot->bitmap_left, target_height - slot->bitmap_top, arguments->text_file);

  /* now, draw to our target surface (convert position)
  draw_bitmap( &slot->bitmap,
                slot->bitmap_left,
                target_height - slot->bitmap_top );

  show_image();
 */

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

  return 0;
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



void draw_to_textfile( FT_Bitmap*  bitmap,FT_Int x, FT_Int y, char* filename)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;

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

  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;

      image[j][i] |= bitmap->buffer[q * bitmap->width + p];
    }
  }

  for ( i = 0; i < HEIGHT; i++ ) {
    for ( j = 0; j < WIDTH; j++ ) {

      char c = (image[i][j] == 0) ? ' ' : ((image[i][j] < 128) ? '+' : '*') ;
      fputc(c, fp);
    }
    fputc('\n', fp);
  }

  fclose(fp);
  fprintf(stdout, "Wrote text bitmap to %s\r\n", filename);

}

void draw_bitmap( FT_Bitmap*  bitmap,FT_Int x, FT_Int y)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;


  /* for simplicity, we assume that `bitmap->pixel_mode' */
  /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;

      image[j][i] |= bitmap->buffer[q * bitmap->width + p];
    }
  }
}


void show_image( void )
{
  int  i, j;


  for ( i = 0; i < HEIGHT; i++ )
  {
    for ( j = 0; j < WIDTH; j++ )
      putchar( image[i][j] == 0 ? ' '
                                : image[i][j] < 128 ? '+'
                                                    : '*' );
    putchar( '\n' );
  }
}
