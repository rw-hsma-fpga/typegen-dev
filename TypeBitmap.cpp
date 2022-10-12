#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <string>
#include <boost/format.hpp> 



TypeBitmap::TypeBitmap()
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL) {}


TypeBitmap::TypeBitmap(std::string filename)
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL)
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

    if (!(pbm >> bm_width) || (bm_width==0)) {
        std::cerr << "ERROR: No valid X dimension" << std::endl;
        return -1;
    }

    // remove comments
    while(pbm.peek()=='#')
        getline(pbm, linebuf);

    if (!(pbm >> bm_height) || (bm_height==0)) {
        std::cerr << "ERROR: No valid Y dimension" << std::endl;
        return -1;
    }

    bitmap = (uint8_t*)calloc(size = bm_width*bm_height, sizeof(uint8_t));
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


int TypeBitmap::set_type_parameters(dim_t TH, dim_t DOD, dim_t RS, dim_t LH)
{
    type_height = TH;
    depth_of_drive = DOD;
    raster_size = RS;
    layer_height = LH;
    return 0;
}


inline void TypeBitmap::STL_triangle_write(std::ofstream &outfile, pos3d_t N, pos3d_t v1, pos3d_t v2, pos3d_t v3, uint32_t &count)
{
    stl_tri_t TRI = (stl_tri_t)
                    {N.x, N.y, N.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, v3.x, v3.y, v3.z, 0};
    outfile.write((const char*)&TRI, 50);
    count++;
}


int TypeBitmap::export_STL(std::string filename)
{
    //const float TYPE_HEIGHT_IN = 0.918;
    //const float MM_PER_INCH = 25.4;
    float RS = raster_size.as_mm();
    float DOD = depth_of_drive.as_mm();
    float BH = type_height.as_mm() - DOD; // Body height in mm

    int x, y;
    int i;
    int w = bm_width;
    int h = bm_height;
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

    stl_out.write((const char*)&tri_cnt, 4); // space for number of triangles

    // normal vectors: X, Y, Z, positive, negative
    pos3d_t Xp = (pos3d_t){ 1,  0,  0};  pos3d_t Xn = (pos3d_t){-1,  0,  0};
    pos3d_t Yp = (pos3d_t){ 0,  1,  0};  pos3d_t Yn = (pos3d_t){ 0, -1,  0};
    pos3d_t Zp = (pos3d_t){ 0,  0,  1};  pos3d_t Zn = (pos3d_t){ 0,  0, -1};

    // cube corners: upper/lower;top/botton;left/right
    pos3d_t utl, utr, ubl, ubr, ltl, ltr, lbl, lbr;

    for (y = 0; y < bm_height; y++) {
        for (x = 0; x < bm_width; x++) {

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
                STL_triangle_write(stl_out, Zp, utr, utl, ubl, tri_cnt);
                STL_triangle_write(stl_out, Zp, utr, ubl, ubr, tri_cnt);

                // left face
                if ((x == 0) || (buf[((y)*w) + (x - 1)] == 0)) {
                    STL_triangle_write(stl_out, Xn, ubl, utl, ltl, tri_cnt);
                    STL_triangle_write(stl_out, Xn, ubl, ltl, lbl, tri_cnt);
                }

                // right face
                if ((x == (bm_width - 1)) || (buf[((y)*w) + (x + 1)] == 0)) {
                    STL_triangle_write(stl_out, Xp, ubr, ltr, utr, tri_cnt);
                    STL_triangle_write(stl_out, Xp, ubr, lbr, ltr, tri_cnt);
                }

                // top face
                if ((y == 0) || (buf[((y - 1) * w) + (x)] == 0)) {
                    STL_triangle_write(stl_out, Yp, utl, utr, ltr, tri_cnt);
                    STL_triangle_write(stl_out, Yp, utl, ltr, ltl, tri_cnt);
                }

                // bottom face
                if ((y == (bm_height - 1)) || (buf[((y + 1) * w) + (x)] == 0)) {
                    STL_triangle_write(stl_out, Yn, ubl, lbr, ubr, tri_cnt);
                    STL_triangle_write(stl_out, Yn, ubl, lbl, lbr, tri_cnt);
                }
            }
            else {
                // upper faces for 0s (body)
                STL_triangle_write(stl_out, Zp, ltr, lbl, ltl, tri_cnt);
                STL_triangle_write(stl_out, Zp, ltr, lbr, lbl, tri_cnt);
            }
        }
    }

    // lower type cube corners - assuming cube goes down from Z=0 to Z=-(type height - depth of drive)

    utl = (pos3d_t){0, 0, 0};
    utr = (pos3d_t){RS * w, 0, 0};
    ubl = (pos3d_t){0, -RS * h, 0};
    ubr = (pos3d_t){RS * w, -RS * h, 0};

    ltl = (pos3d_t){0, 0, -BH};
    ltr = (pos3d_t){RS * w, 0, -BH};
    lbl = (pos3d_t){0, -RS * h, -BH};
    lbr = (pos3d_t){RS * w, -RS * h, -BH};

    // lower face
    STL_triangle_write(stl_out, Zn, ltr, lbl, ltl, tri_cnt);
    STL_triangle_write(stl_out, Zn, ltr, lbr, lbl, tri_cnt);

    // left face
    STL_triangle_write(stl_out, Xn, ubl, utl, ltl, tri_cnt);
    STL_triangle_write(stl_out, Xn, ubl, ltl, lbl, tri_cnt);

    // right face
    STL_triangle_write(stl_out, Xp, ubr, ltr, utr, tri_cnt);
    STL_triangle_write(stl_out, Xp, ubr, lbr, ltr, tri_cnt);

    // top face
    STL_triangle_write(stl_out, Yp, utl, utr, ltr, tri_cnt);
    STL_triangle_write(stl_out, Yp, utl, ltr, ltl, tri_cnt);

    // bottom face
    STL_triangle_write(stl_out, Yn, ubl, lbr, ubr, tri_cnt);
    STL_triangle_write(stl_out, Yn, ubl, lbl, lbr, tri_cnt);

    std::cout << "Triangle count is " << tri_cnt << std::endl;

    stl_out.seekp(80);
    stl_out.write((const char*)&tri_cnt, 4);
    stl_out.close();

    std::cout << "Wrote binary STL data to " << filename << std::endl;
    std::cout << "---------------------" << std::endl;
    std::cout << "Exported STL metrics:" << std::endl;
    std::cout << "Type height   " << boost::format("%6.4f") % type_height.as_inch()
              << " inch  |  "  << boost::format("%6.3f") %  type_height.as_mm() << " mm" << std::endl;
    std::cout << "Body size     " << boost::format("%6.4f") % (h*raster_size.as_inch())
              << " inch  |  "  << boost::format("%6.3f") %  (h*raster_size.as_mm()) << " mm" << std::endl;
    std::cout << "Set width     " << boost::format("%6.4f") % (w*raster_size.as_inch())
              << " inch  |  "  << boost::format("%6.3f") %  (w*raster_size.as_mm()) << " mm" << std::endl;

    return 0;
}