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


typedef struct pos3d
{
    float x, y, z;
} pos3d_t;


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


// define for effective inlining
#define STL_triangle_write(N, v1, v2, v3)                                                                    \
  ({                                                                                                         \
    TRI = (struct stl_binary_triangle){N.x, N.y, N.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, 0}; \
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

    unsigned char *buf = (unsigned char*)bitmap;


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


    struct stl_binary_triangle TRI;

    stl_out.write((const char*)&tri_cnt, 4); // space for number of triangles

  // cube corners: upper/lower;top/botton;left/right

    pos3d_t utl, utr, ubl, ubr, ltl, ltr, lbl, lbr;

    // normal vectors: X, Y, Z, positive, negative
    pos3d_t Xp = (pos3d_t){ 1,  0,  0};  pos3d_t Xn = (pos3d_t){-1,  0,  0};
    pos3d_t Yp = (pos3d_t){ 0,  1,  0};  pos3d_t Yn = (pos3d_t){ 0, -1,  0};
    pos3d_t Zp = (pos3d_t){ 0,  0,  1};  pos3d_t Zn = (pos3d_t){ 0,  0, -1};

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {

            // pixel cube corners - assuming cubes are going up from Z=0 to Z=+(depth of drive)
            utl = (pos3d_t){RS * x, -RS * y, DOD};
            utr = (pos3d_t){RS * (x + 1), -RS * y, DOD};
            ubl = (pos3d_t){RS * x, -RS * (y + 1), DOD};
            ubr = (pos3d_t){RS * (x + 1), -RS * (y + 1), DOD};

            ltl = (pos3d_t){RS * x, -RS * y, 0};
            ltr = (pos3d_t){RS * (x + 1), -RS * y, 0};
            lbl = (pos3d_t){RS * x, -RS * (y + 1), 0};
            lbr = (pos3d_t){RS * (x + 1), -RS * (y + 1), 0};

            if (buf[y * w + x]) {

                // upper face
                STL_triangle_write(Zp, utr, utl, ubl);
                STL_triangle_write(Zp, utr, ubl, ubr);

                // left face
                if ((x == 0) || (buf[((y)*w) + (x - 1)] == 0)) {
                    STL_triangle_write(Xn, ubl, utl, ltl);
                    STL_triangle_write(Xn, ubl, ltl, lbl);
                }

                // right face
                if ((x == (width - 1)) || (buf[((y)*w) + (x + 1)] == 0)) {
                    STL_triangle_write(Xp, ubr, ltr, utr);
                    STL_triangle_write(Xp, ubr, lbr, ltr);
                }

                // top face
                if ((y == 0) || (buf[((y - 1) * w) + (x)] == 0)) {
                    STL_triangle_write(Yp, utl, utr, ltr);
                    STL_triangle_write(Yp, utl, ltr, ltl);
                }

                // bottom face
                if ((y == (height - 1)) || (buf[((y + 1) * w) + (x)] == 0)) {
                    STL_triangle_write(Yn, ubl, lbr, ubr);
                    STL_triangle_write(Yn, ubl, lbl, lbr);
                }
            }
            else {
                // upper faces for 0s (body)
                STL_triangle_write(Zp, ltr, lbl, ltl);
                STL_triangle_write(Zp, ltr, lbr, lbl);
            }
        }
    }

    // lower type cupe corners - assuming cube goes down from Z=0 to Z=-(type height - depth of drive)

    utl = (pos3d_t){0, 0, 0};
    utr = (pos3d_t){RS * w, 0, 0};
    ubl = (pos3d_t){0, -RS * h, 0};
    ubr = (pos3d_t){RS * w, -RS * h, 0};

    ltl = (pos3d_t){0, 0, -BH};
    ltr = (pos3d_t){RS * w, 0, -BH};
    lbl = (pos3d_t){0, -RS * h, -BH};
    lbr = (pos3d_t){RS * w, -RS * h, -BH};

    // lower face
    STL_triangle_write(Zn, ltr, lbl, ltl);
    STL_triangle_write(Zn, ltr, lbr, lbl);

    // left face
    STL_triangle_write(Xn, ubl, utl, ltl);
    STL_triangle_write(Xn, ubl, ltl, lbl);

    // right face
    STL_triangle_write(Xp, ubr, ltr, utr);
    STL_triangle_write(Xp, ubr, lbr, ltr);

    // top face
    STL_triangle_write(Yp, utl, utr, ltr);
    STL_triangle_write(Yp, utl, ltr, ltl);

    // bottom face
    STL_triangle_write(Yn, ubl, lbr, ubr);
    STL_triangle_write(Yn, ubl, lbl, lbr);

    std::cout << "Triangle count is " << tri_cnt << std::endl;

    stl_out.seekp(80);
    stl_out.write((const char*)&tri_cnt, 4);
    stl_out.close();


    std::cout << "Wrote binary STL data to" << filename << std::endl;
    fprintf(stdout, "---------------------\r\n");
    fprintf(stdout, "Exported STL metrics:\r\n");
    fprintf(stdout, "Type height   %6.4f inch  |  %6.3f mm\r\n", TYPE_HEIGHT_IN, TYPE_HEIGHT_IN*MM_PER_INCH);
    fprintf(stdout, "Body size     %6.4f inch  |  %6.3f mm\r\n", (h*RS)/MM_PER_INCH, h*RS);
    fprintf(stdout, "Set width     %6.4f inch  |  %6.3f mm\r\n", (w*RS)/MM_PER_INCH, w*RS);


    return 0;
}