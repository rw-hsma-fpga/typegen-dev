#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>

TypeBitmap::TypeBitmap() : loaded(false), width(0), height(0), bitmap(NULL) {}

TypeBitmap::TypeBitmap(std::string filename) : loaded(false), width(0), height(0), bitmap(NULL)
{
    unload();
    load(filename);
}

TypeBitmap::~TypeBitmap() {
    unload();
}

int TypeBitmap::load(std::string filename)
{
    std::string linebuf;
    uint32_t size;

    std::ifstream pbm(filename);
    if (!pbm.is_open())
        return -1;

    getline(pbm, linebuf);
    std::erase(linebuf, '\r');
    if (linebuf!="P1") {
        std::cerr << "ERROR: Not a valid PBM ASCII file!" << std::endl;
        return -1;
    }

    // remove comments
    while(pbm.peek()=='#')
        getline(pbm, linebuf);

    if (!(pbm >> width) || (width==0)) {
        std::cerr << "ERROR: No valid X dimension" << std::endl;
        return -1;
    }

    // remove comments
    while(pbm.peek()=='#')
        getline(pbm, linebuf);

    if (!(pbm >> height) || (height==0)) {
        std::cerr << "ERROR: No valid Y dimension" << std::endl;
        return -1;
    }

    bitmap = (uint8_t*)calloc(size = width*height, sizeof(uint8_t));
    if (bitmap == NULL) {
        std::cerr << "ERROR: Bitmap buffer allocation failed" << std::endl;
        return -1;
    }

    char c;
    uint8_t *runbuf = bitmap;
    uint32_t count = 0;

    while(!pbm.eof()) {
        c = pbm.get();
        switch(c) {
            case '#':
                getline(pbm, linebuf); break;
            case '0':
            case '1':
                if (count == size) {
                    std::cerr << "ERROR: More data than specified." << std::endl;
                    free(bitmap);
                    bitmap = NULL;
                    return -1;
                }

                if (c=='1')
                    *(runbuf++) = 255;
                else
                    *(runbuf++) = 0;

                count++;
            default:
                break;
        }
    }

    if (count != size) {
        std::cerr << "Less data than specified." << std::endl;
        free(bitmap);
        bitmap = NULL;
        return -1;
    }

    loaded = true;
    return 0;
}

void TypeBitmap::unload()
{
    if (bitmap != NULL) {
        free(bitmap);
    }
    bitmap = NULL;
    loaded = false;
}

bool TypeBitmap::is_loaded()
{
    return loaded;
}


// define for effective inlining
#define STL_triangle_write(Nx, Ny, Nz,                                                                       \
                           v1, v2, v3)                                                                       \
  ({                                                                                                         \
    TRI = (struct stl_binary_triangle){Nx, Ny, Nz, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, 0}; \
    stl_out.write((const char*)&TRI, 50); \
    tri_cnt++;                                                                                               \
  })


int TypeBitmap::export_STL(std::string filename)
{
    const float TYPE_HEIGHT_IN = 0.918;
    const float MM_PER_INCH = 25.4;
    const float RS = 0.0285; // raster size in mm (28,5um)
    const float DOD = 1.6;   // Depth of drive in mm

    float BH = TYPE_HEIGHT_IN * MM_PER_INCH - DOD; // Body height in mm

    int x, y;
    int i;
    int w = width;
    int h = height;
    unsigned int tri_cnt = 0;

    if (filename.empty())
    {
        std::cerr << "No STL file specified." << std::endl;
        return -1;
    }

    std::ofstream stl_out(filename, std::ios::binary);
    if (!stl_out.is_open()) {
        std::cerr << "ERROR: Could not open STL file for writing." << std::endl;
        return -1;
    }    

    // 80 byte header - content anything but "solid" (would indicated ASCII encoding)
    for (i = 0; i < 80; i++)
        stl_out.put('x');

    struct stl_binary_triangle
    {
        float Nx;
        float Ny;
        float Nz;
        float V1x;
        float V1y;
        float V1z;
        float V2x;
        float V2y;
        float V2z;
        float V3x;
        float V3y;
        float V3z;
        short int attr_cnt;
    };

    struct stl_binary_triangle TRI;

    stl_out.write((const char*)&tri_cnt, 4); // space for number of triangles

  // cube corners: upper/lower;top/botton;left/right
  typedef struct corner
  {
    float x, y, z;
  } corner_t;
  corner_t utl, utr, ubl, ubr, ltl, ltr, lbl, lbr;

  unsigned char *buf = (unsigned char*)bitmap;

  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {

      utl = (corner_t){RS * x, -RS * y, DOD};
      utr = (corner_t){RS * (x + 1), -RS * y, DOD};
      ubl = (corner_t){RS * x, -RS * (y + 1), DOD};
      ubr = (corner_t){RS * (x + 1), -RS * (y + 1), DOD};

      ltl = (corner_t){RS * x, -RS * y, 0};
      ltr = (corner_t){RS * (x + 1), -RS * y, 0};
      lbl = (corner_t){RS * x, -RS * (y + 1), 0};
      lbr = (corner_t){RS * (x + 1), -RS * (y + 1), 0};

      // fprintf(stdout, ".. X: %d   Y: %d\r\n", x, y);

      if (buf[y * w + x])
      {

        // fprintf(stdout, "X: %d   Y: %d\r\n", x, y);

        // upper face
        STL_triangle_write(0, 0, 1, utr, utl, ubl);
        STL_triangle_write(0, 0, 1, utr, ubl, ubr);

        // left face
        if ((x == 0) || (buf[((y)*w) + (x - 1)] == 0))
        {
          STL_triangle_write(-1, 0, 0, ubl, utl, ltl);
          STL_triangle_write(-1, 0, 0, ubl, ltl, lbl);
        }

        // right face
        if ((x == (width - 1)) || (buf[((y)*w) + (x + 1)] == 0))
        {
          STL_triangle_write(1, 0, 0, ubr, ltr, utr);
          STL_triangle_write(1, 0, 0, ubr, lbr, ltr);
        }

        // top face
        if ((y == 0) || (buf[((y - 1) * w) + (x)] == 0))
        {
          STL_triangle_write(0, 1, 0, utl, utr, ltr);
          STL_triangle_write(0, 1, 0, utl, ltr, ltl);
        }

        // bottom face
        if ((y == (height - 1)) || (buf[((y + 1) * w) + (x)] == 0))
        {
          STL_triangle_write(0, -1, 0, ubl, lbr, ubr);
          STL_triangle_write(0, -1, 0, ubl, lbl, lbr);
        }
      }
      else
      {
        // upper faces for 0s (body)
        STL_triangle_write(0, 0, 1, ltr, lbl, ltl);
        STL_triangle_write(0, 0, 1, ltr, lbr, lbl);
      }
    }
  }

  utl = (corner_t){0, 0, 0};
  utr = (corner_t){RS * w, 0, 0};
  ubl = (corner_t){0, -RS * h, 0};
  ubr = (corner_t){RS * w, -RS * h, 0};

  ltl = (corner_t){0, 0, -BH};
  ltr = (corner_t){RS * w, 0, -BH};
  lbl = (corner_t){0, -RS * h, -BH};
  lbr = (corner_t){RS * w, -RS * h, -BH};

  // lower face
  STL_triangle_write(0, 0, -1, ltr, lbl, ltl);
  STL_triangle_write(0, 0, -1, ltr, lbr, lbl);

  // left face
  STL_triangle_write(-1, 0, 0, ubl, utl, ltl);
  STL_triangle_write(-1, 0, 0, ubl, ltl, lbl);

  // right face
  STL_triangle_write(1, 0, 0, ubr, ltr, utr);
  STL_triangle_write(1, 0, 0, ubr, lbr, ltr);

  // top face
  STL_triangle_write(0, 1, 0, utl, utr, ltr);
  STL_triangle_write(0, 1, 0, utl, ltr, ltl);

  // bottom face
  STL_triangle_write(0, -1, 0, ubl, lbr, ubr);
  STL_triangle_write(0, -1, 0, ubl, lbl, lbr);

    std::cout << "Triangle count is " << tri_cnt << std::endl;

    stl_out.seekp(80);
    stl_out.write((const char*)&tri_cnt, 4);
    stl_out.close();





    std::cout << "Wrote binary STL data to" << " __" << filename << std::endl;
    fprintf(stdout, "---------------------\r\n");
    fprintf(stdout, "Exported STL metrics:\r\n");
    fprintf(stdout, "Type height   %6.4f inch  |  %6.3f mm\r\n", TYPE_HEIGHT_IN, TYPE_HEIGHT_IN*MM_PER_INCH);
    fprintf(stdout, "Body size     %6.4f inch  |  %6.3f mm\r\n", (h*RS)/MM_PER_INCH, h*RS);
    fprintf(stdout, "Set width     %6.4f inch  |  %6.3f mm\r\n", (w*RS)/MM_PER_INCH, w*RS);


    return 0;
}