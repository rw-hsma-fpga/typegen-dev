#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <boost/format.hpp> 



TypeBitmap::TypeBitmap()
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL), tag_bitmap_i32(NULL) {}


TypeBitmap::TypeBitmap(std::string filename)
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL), tag_bitmap_i32(NULL)
{
    unload();
    load(filename);
}

TypeBitmap::TypeBitmap(uint32_t width, uint32_t height)
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL), tag_bitmap_i32(NULL)
{
    newBitmap(width, height);
}

int TypeBitmap::newBitmap(uint32_t width, uint32_t height)
{
    unload();
    bm_width = width;
    bm_height = height;

    bitmap = (uint8_t*)calloc(bm_width*bm_height, sizeof(uint8_t));

    if (bitmap != NULL) {
        loaded = true;
        return 0;
    }
    else {
        return -1;
    }
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
        pbm.close();
        return -1;
    }

    // remove comments
    while(pbm.peek()=='#')
        getline(pbm, linebuf);

    if (!(pbm >> bm_width) || (bm_width==0)) {
        std::cerr << "ERROR: No valid X dimension" << std::endl;
        pbm.close();
        return -1;
    }

    // remove comments
    while(pbm.peek()=='#')
        getline(pbm, linebuf);

    if (!(pbm >> bm_height) || (bm_height==0)) {
        std::cerr << "ERROR: No valid Y dimension" << std::endl;
        pbm.close();
        return -1;
    }

    bitmap = (uint8_t*)calloc(size = bm_width*bm_height, sizeof(uint8_t));

    if (bitmap == NULL) {
        std::cerr << "ERROR: Bitmap buffer allocation failed" << std::endl;
        pbm.close();
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
                    pbm.close();
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
        pbm.close();
        return -1;
    }

    loaded = true;
    pbm.close();
    return 0;
}


void TypeBitmap::unload()
{
    if (tag_bitmap_i32 != NULL) {
        free(tag_bitmap_i32);
    }
    tag_bitmap_i32 = NULL;

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


int TypeBitmap::store(std::string filename)
{
    uint8_t *bm_ptr;
    uint32_t x, y;

    if (!loaded) {
        std::cerr << "ERROR: No bitmap to store." << std::endl;
        return -1;
    }

    std::ofstream pbm(filename);
    if (!pbm.is_open()) {
        std::cerr << "ERROR: Opening " << filename <<" for writing failed." << std::endl;
        return -1;
    }

    bm_ptr = bitmap;

    pbm << "P1" << std::endl;
    pbm << bm_width << " " << bm_height << std::endl;

    for (y=0; y<bm_height ; y++) {
        for (x=0; x<bm_width; x++) {
            if (*bm_ptr++)
                pbm << "1 ";
            else
                pbm << "0 ";
        }
        pbm << std::endl;
    }

    pbm.close();
    return 0;
}

int TypeBitmap::pasteGlyph(uint8_t *glyph, uint32_t g_width, uint32_t g_height, uint32_t top_pos, uint32_t left_pos)
{
    int g_x, g_y, bm_x, bm_y;
    
    bool glyph_fits = true;
    
    if (glyph == NULL) {
        std::cout << "ERROR: No glyph allocated to paste." << std::endl;
        return -1;
    }

    for (g_y=0 ; g_y<g_height; g_y++) {
        bm_y = g_y + top_pos;
        if (bm_y >= bm_height) {
            glyph_fits = false;
            continue;
        }

        for (g_x=0 ; g_x<g_width; g_x++) {
            bm_x = g_x + left_pos;
            if (bm_x >= bm_width) {
                glyph_fits = false;
                continue;
            }
            
            bitmap[bm_y*bm_width + bm_x] = glyph[g_y*g_width + g_x];
        }
    }

    if (!glyph_fits) {
        std::cout << "WARNING: Glyph didn't fit into bitmap." << std::endl;
        return -1;
    }

    return 0;
}

void TypeBitmap::threshold(uint8_t thr) {

    uint32_t x, y;
    const uint32_t w = bm_width;
    const uint32_t h = bm_height;

    uint8_t *buf = bitmap;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < (w >> 1); x++)
        {
            if (buf[y * w + x] >= thr)
                buf[y * w + x] = 255; // 1?
            else
                buf[y * w + x] = 0;
        }
    }
    return;
}

void TypeBitmap::mirror() {

    uint32_t x, y;
    uint32_t w = bm_width;
    uint32_t h = bm_height;

    uint8_t *buf = bitmap;
    uint8_t swap;

    if (!loaded)
        return; //-1;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < (w >> 1); x++)
        {
            int indexA = y * w + x;
            int indexB = y * w + ((w-1) - x);

            swap = buf[indexA];
            buf[indexA] = buf[indexB];
            buf[indexB] = swap;
        }
    }
    return;
}


int TypeBitmap::set_type_parameters(dim_t TH, dim_t DOD, dim_t RS, dim_t LH)
{
    type_height = TH;
    depth_of_drive = DOD;
    raster_size = RS;
    layer_height = LH;

    return 0;
}


void TypeBitmap::OBJ_rect_push(intvec3d_t N, intvec3d_t v1, intvec3d_t v2, intvec3d_t v3, intvec3d_t v4)
{
    float twoPi = 8*atan(1);
    float halfPi = 2*atan(1);

    struct prj2d_t {
        intvec3d_t *vert3d;
        float px;
        float py;
        float angle;
        uint8_t pos;
    };

    prj2d_t proj[4];
    proj[0].vert3d = &v1;
    proj[0].pos = 0;

    proj[1].vert3d = &v2;
    proj[1].pos = 1;

    proj[2].vert3d = &v3;
    proj[2].pos = 2;

    proj[3].vert3d = &v4;
    proj[3].pos = 3;

    pos3d_t Nf = (pos3d_t) {.x = float(N.x), .y = float(N.y), .z = float(N.z)};
    float cosrY, sinrY, cosrX, sinrX;

    if ((Nf.z==0) && (Nf.x==0)) { // don't turn around Y
        cosrY = 1;
        sinrY = 0;
    }
    else {
        // get X-to-Z arc (rotate around Y)
        //normalize x
        float x_unit = Nf.x / sqrtf(Nf.x*Nf.x + Nf.z*Nf.z);
        // get angle from X-axis into XZ plane
        float xz_angle = acosf(x_unit);
        if (Nf.z < 0) {
            xz_angle = twoPi - xz_angle;
        }
        float rotY = -(halfPi - xz_angle);

        cosrY = cosf(rotY);
        sinrY = sinf(rotY);
    }
    // rotate around Y
    pos3d_t Nb;
    Nb.x = Nf.x*cosrY + Nf.z*sinrY;
    Nb.y = Nf.y;
    Nb.z = -Nf.x*sinrY + Nf.z*cosrY;
    //std::cout << "Norm after Y rot:  x=" << Nb.x << ", y=" << Nb.y << ", z=" << Nb.z << std::endl;

    if ((Nb.z==0) && (Nb.y==0)) { // don't turn around X
        cosrX = 1;
        sinrX = 0;
    }
    else {
        // get Y-to-Z arc (rotate around X)
        //normalize y
        float y_unit = Nb.y / sqrtf(Nb.y*Nb.y + Nb.z*Nb.z);
        // get angle from Y-axis into XZ plane
        float yz_angle = acosf(y_unit);
        if (Nb.z < 0) {
            yz_angle = twoPi - yz_angle;
        }
        float rotX = (halfPi - yz_angle);
        cosrX = cosf( rotX );
        sinrX = sinf( rotX );
    }

    /* full two-step X-Y for the record
        pos3d_t P1 = { 1, 1, 0 };
        Nb.x =  P1.x*cosrY + P1.z*sinrY;
        Nb.y =  P1.y;
        Nb.z = -P1.x*sinrY + P1.z*cosrY;
        P1.x = Nb.x;
        P1.y = Nb.y*cosrX - Nb.z*sinrX;
        P1.z = Nb.y*sinrX + Nb.z*cosrX;
        std::cout << "P1 after Y-X rot:  x=" << P1.x << ", y=" << P1.y << ", z=" << P1.z << std::endl;
    */

    for (int i=0; i<4; i++) {
        float x = float(proj[i].vert3d->x);
        float y = float(proj[i].vert3d->y);
        float z = float(proj[i].vert3d->z);
        proj[i].px = x*cosrY + z*sinrY;
        proj[i].py = y*cosrX - (-x*sinrY + z*cosrY)*sinrX;
    }

    float center_x = (proj[0].px + proj[1].px + proj[2].px + proj[3].px) / 4;
    float center_y = (proj[0].py + proj[1].py + proj[2].py + proj[3].py) / 4;

    for (int i=0; i<4; i++) {
        float deltax = proj[i].px - center_x;
        float deltay = proj[i].py - center_y;
        float norm = sqrt(deltax*deltax + deltay*deltay);
        deltax /= norm;
        float ang = acos(deltax);
        if (deltay < 0)
            ang = twoPi - ang; // 0..2pi position at this point

        proj[i].angle = ang;
        if (i!=0) {
            if (proj[i].angle < proj[0].angle)
                proj[i].angle += twoPi;
        }
        
    }

    // bubble sort 1..3 
    uint8_t swap_pos;
    float swap_angle;
    intvec3d_t *swap_vert3d;

    for (int i=0; i<3; i++) {
        int j = (i % 2)+1; // 1-2,2-3,1-2,done
        if (proj[j].angle > proj[j+1].angle) {
            swap_pos    = proj[j].pos;
            swap_angle  = proj[j].angle;
            swap_vert3d = proj[j].vert3d;
            proj[j].pos    = proj[j+1].pos;
            proj[j].angle  = proj[j+1].angle;
            proj[j].vert3d = proj[j+1].vert3d;
            proj[j+1].pos    = swap_pos;
            proj[j+1].angle  = swap_angle;
            proj[j+1].vert3d = swap_vert3d;
        }
    }

    mesh_triangle TRI;

    TRI.N = N;
    TRI.v1 = find_or_add_vertex(*(proj[0].vert3d));
    TRI.v2 = find_or_add_vertex(*(proj[1].vert3d));
    TRI.v3 = find_or_add_vertex(*(proj[2].vert3d));
    triangles.push_back(TRI);

    TRI.N = N;
    TRI.v1 = find_or_add_vertex(*(proj[0].vert3d));
    TRI.v2 = find_or_add_vertex(*(proj[2].vert3d));
    TRI.v3 = find_or_add_vertex(*(proj[3].vert3d));
    triangles.push_back(TRI);
}


uint32_t TypeBitmap::find_or_add_vertex(intvec3d_t v)
{
    int i;

    for(i=1; i<vertices.size(); i++) {
        if ((vertices[i].x == v.x) &&
            (vertices[i].y == v.y) &&
            (vertices[i].z == v.z) ) {
            return i;
        }
    }
    // didn't find it, so make new one:
    vertices.push_back(v);
    return i; // new vertex' index
}


int TypeBitmap::generateMesh(reduced_foot foot, std::vector<nick> &nicks, float UVstretchZ)
{
    int x, y;
    int i, j, k;
    int w = bm_width;
    int h = bm_height;

    vertices.clear();
    intvec3d_t vec3dbuf = (intvec3d_t) {INT32_MIN, INT32_MIN, INT32_MIN};
    vertices.push_back(vec3dbuf);

    triangles.clear();

    uint8_t *buf8 = (uint8_t*)bitmap;
    int32_t *buf32;

    float RS = raster_size.as_mm();
    float LH = layer_height.as_mm();

    int32_t DOD = int32_t( round( (UVstretchZ*depth_of_drive.as_mm())/LH ) );
    int32_t BH = int32_t( round( (UVstretchZ*(type_height.as_mm()-depth_of_drive.as_mm()))/LH ) );

    int32_t FZ;
    int32_t FXY;

    if (foot.mode == no_foot) {
        FZ = 0;
        FXY = 0;
    }
    else {
        FZ = int32_t( round( (foot.Z.as_mm() * UVstretchZ)/LH ) );
        FXY = int32_t( round( foot.XY.as_mm()/RS  ) );
    }


    if (!loaded)
    {
        std::cerr << "ERROR: No Bitmap loaded." << std::endl;
        return -1;
    }


    glyph_rects.clear();
    body_rects.clear();

    tag_bitmap_i32 = (int32_t*)calloc(w*h, sizeof(int32_t));
    if (tag_bitmap_i32 == NULL) {
        std::cerr << "ERROR: Could not open allocate tag bitmap." << std::endl;
        return -1;
    }
    buf32 = tag_bitmap_i32;

    for (y = 0; y < bm_height; y++) {
        for (x = 0; x < bm_width; x++) {
            if (*buf8++)
                *buf32++ = +1;
            else
                *buf32++ = -1;
        }
    }
    buf32 = tag_bitmap_i32;

    // pseudo randomized loop
    srand(0xB747);
    int32_t tag_cnt = 2;
    
    const int ITERATIONS = 100000;

    for (i = 0; i < ITERATIONS; i++) {
        uint32_t rand_x = rand() % w;
        uint32_t rand_y = rand() % h;

        // check if rect'ed already
        // expand rect
        int32_t val = buf32[rand_y*w + rand_x];

        if ((val != +1) && (val != -1))
            continue; // already rect'ed

        // expansion algorithm
        bool expanded = false;

        bool left_end = false;
        bool right_end = false;
        bool top_end = false;
        bool bottom_end = false;

        STLrect valrect;
        valrect.left = rand_x;
        valrect.right = rand_x;
        valrect.top = rand_y;
        valrect.bottom = rand_y;
        if (val==+1)
            valrect.tag =  tag_cnt;
        else
            valrect.tag =  -tag_cnt;

        int index;

        while(!left_end || !right_end || !top_end || !bottom_end) {

            if (!left_end)
                if (valrect.left==0)
                    left_end = true;
                else {
                    index = (valrect.top*w) + (valrect.left-1);
                    for (j=valrect.top; j<=valrect.bottom; j++) {
                        if (buf32[index]!=val)
                            break;
                        index += w;
                    }
                    if (j<=valrect.bottom) { // failed
                        left_end = true;
                    }
                    else {
                        valrect.left--;
                        expanded =  true;
                    }
                }

            if (!top_end)
                if (valrect.top==0)
                    top_end = true;
                else {
                    index = ((valrect.top-1)*w) + (valrect.left);
                    for (k=valrect.left; k<=valrect.right; k++) {
                        if (buf32[index]!=val)
                            break;
                        index += 1;
                    }
                    if (k<=valrect.right) { // failed
                        top_end = true;
                    }
                    else {
                        valrect.top--;
                        expanded =  true;
                    }
                }

            if (!right_end)
                if (valrect.right==w-1)
                    right_end = true;
                else {
                    index = (valrect.top*w) + (valrect.right+1);
                    for (j=valrect.top; j<=valrect.bottom; j++) {
                        if (buf32[index]!=val)
                            break;
                        index += w;
                    }
                    if (j<=valrect.bottom) { // failed
                        right_end = true;
                    }
                    else {
                        valrect.right++;
                        expanded =  true;
                    }
                }

            if (!bottom_end)
                if (valrect.bottom==h-1)
                    bottom_end = true;
                else {
                    index = ((valrect.bottom+1)*w) + (valrect.left);
                    for (k=valrect.left; k<=valrect.right; k++) {
                        if (buf32[index]!=val)
                            break;
                        index += 1;
                    }
                    if (k<=valrect.right) { // failed
                        bottom_end = true;
                    }
                    else {
                        valrect.bottom++;
                        expanded =  true;
                    }
                }
        }

        if (expanded) {
            if (val==-1) {
                body_rects.push_back(valrect);
            }
            if (val==+1) {
                glyph_rects.push_back(valrect);
            }

            buf32 = tag_bitmap_i32;
            buf32 += (valrect.top*w);
            for (j=valrect.top; j<=valrect.bottom; j++) {
                for (k=valrect.left; k<=valrect.right; k++) {
                    buf32[k] = valrect.tag;
                }
                buf32 += w;
            }
            buf32 = tag_bitmap_i32;


            tag_cnt++;
        }
    }


    // normal vectors: X, Y, Z, positive, negative
    intvec3d_t Xp = (intvec3d_t){ 1,  0,  0};  intvec3d_t Xn = (intvec3d_t){-1,  0,  0};
    intvec3d_t Yp = (intvec3d_t){ 0,  1,  0};  intvec3d_t Yn = (intvec3d_t){ 0, -1,  0};
    intvec3d_t Zp = (intvec3d_t){ 0,  0,  1};  intvec3d_t Zn = (intvec3d_t){ 0,  0, -1};

    // normal vectors: - 45 degrees downwards
    intvec3d_t LD = (intvec3d_t){-1,  0, -1}; // left down
    intvec3d_t RD = (intvec3d_t){ 1,  0, -1}; // right down
    intvec3d_t TD = (intvec3d_t){ 0,  1, -1}; // top down
    intvec3d_t BD = (intvec3d_t){ 0, -1, -1}; // bottom down

    // cube corners: upper/lower;top/botton;left/right
    intvec3d_t utl, utr, ubl, ubr, ltl, ltr, lbl, lbr;


    // LARGE RECTS 
    for (i = 0; i < body_rects.size(); i++) {
            STLrect R = body_rects[i];

            ltl = (intvec3d_t){R.left, -R.top, 0};
            ltr = (intvec3d_t){(R.right + 1), -R.top, 0};
            lbl = (intvec3d_t){R.left, -(R.bottom + 1), 0};
            lbr = (intvec3d_t){(R.right + 1), -(R.bottom + 1), 0};

            OBJ_rect_push(Zp, ltr, lbl, ltl, lbr);
    }

    for (i = 0; i < glyph_rects.size(); i++) {
            STLrect R = glyph_rects[i];

            utl = (intvec3d_t){R.left, -R.top, DOD};
            utr = (intvec3d_t){(R.right + 1), -R.top, DOD};
            ubl = (intvec3d_t){R.left, -(R.bottom + 1), DOD};
            ubr = (intvec3d_t){(R.right + 1), -(R.bottom + 1), DOD};

            OBJ_rect_push(Zp, utr, utl, ubl, ubr);
    }




    // SINGLE PIXELS
    for (y = 0; y < bm_height; y++) {
        for (x = 0; x < bm_width; x++) {

            // pixel cube corners - assuming cubes are going up from Z=0 to Z=+(depth of drive)
            utl = (intvec3d_t){x, -y, DOD};
            utr = (intvec3d_t){(x + 1), -y, DOD};
            ubl = (intvec3d_t){x, -(y + 1), DOD};
            ubr = (intvec3d_t){(x + 1), -(y + 1), DOD};

            ltl = (intvec3d_t){x, -y, 0};
            ltr = (intvec3d_t){(x + 1), -y, 0};
            lbl = (intvec3d_t){x, -(y + 1), 0};
            lbr = (intvec3d_t){(x + 1), -(y + 1), 0};

            // single pixel upper faces (glyph)
            if (buf32[y * w + x] == +1) {
                // upper face
                OBJ_rect_push(Zp, utr, utl, ubl, ubr);
            }

            // single pixel upper faces (body / no glyph)
            if (buf32[y * w + x] == -1) {
                // lower faces become upper faces of body for 0
                OBJ_rect_push(Zp, ltr, lbl, ltl, lbr);
            }

            // side walls of glyph
            if (buf32[y * w + x] > 0) {
                

                // left face
                if ((x == 0) || (buf32[((y)*w) + (x - 1)] < 0)) {
                    OBJ_rect_push(Xn, ubl, utl, ltl, lbl);
                }

                // right face
                if ((x == (bm_width - 1)) || (buf32[((y)*w) + (x + 1)] < 0)) {
                    OBJ_rect_push(Xp, ubr, ltr, utr, lbr);
                }

                // top face
                if ((y == 0) || (buf32[((y - 1) * w) + (x)] < 0)) {
                    OBJ_rect_push(Yp, utl, utr, ltr, ltl);
                }

                // bottom face
                if ((y == (bm_height - 1)) || (buf32[((y + 1) * w) + (x)] < 0)) {
                    OBJ_rect_push(Yn, ubl, lbr, ubr, lbl);
                }
            }
        }
    }



    // BODY

    // type body cube corners - assuming cube goes down from Z=0 to Z=-(type height - depth of drive)

    utl = (intvec3d_t){0,  0, 0};
    utr = (intvec3d_t){w,  0, 0};
    ubl = (intvec3d_t){0, -h, 0};
    ubr = (intvec3d_t){w, -h, 0};

    ltl = (intvec3d_t){0,  0, -(BH-FZ)};
    ltr = (intvec3d_t){w,  0, -(BH-FZ)};
    lbl = (intvec3d_t){0, -h, -(BH-FZ)};
    lbr = (intvec3d_t){w, -h, -(BH-FZ)};

    // left face
    OBJ_rect_push(Xn, ubl, utl, ltl, lbl);

    // right face
    OBJ_rect_push(Xp, ubr, ltr, utr, lbr);

    // top face
    OBJ_rect_push(Yp, utl, utr, ltr, ltl);

    // bottom face
    OBJ_rect_push(Yn, ubl, lbr, ubr, lbl);


    // lower(lowest) face - bevel/step foot
    if ((foot.mode == step) || (foot.mode == bevel)) {
        ltl = (intvec3d_t){FXY,         -FXY, -BH};
        ltr = (intvec3d_t){w - FXY,     -FXY, -BH};
        lbl = (intvec3d_t){FXY,     -h + FXY, -BH};
        lbr = (intvec3d_t){w - FXY, -h + FXY, -BH};
    }

    if (foot.mode == step) { // stepped foot
        utl = (intvec3d_t){FXY,         -FXY, -(BH-FZ)};
        utr = (intvec3d_t){w - FXY,     -FXY, -(BH-FZ)};
        ubl = (intvec3d_t){FXY,     -h + FXY, -(BH-FZ)};
        ubr = (intvec3d_t){w - FXY, -h + FXY, -(BH-FZ)};

        // left face
        OBJ_rect_push(Xn, ubl, utl, ltl, lbl);

        // right face
        OBJ_rect_push(Xp, ubr, ltr, utr, lbr);

        // top face
        OBJ_rect_push(Yp, utl, utr, ltr, ltl);

        // bottom face
        OBJ_rect_push(Yn, ubl, lbr, ubr, lbl);

    }

    if (foot.mode == bevel) { // beveled foot
        utl = (intvec3d_t){0,  0, -(BH-FZ)};
        utr = (intvec3d_t){w,  0, -(BH-FZ)};
        ubl = (intvec3d_t){0, -h, -(BH-FZ)};
        ubr = (intvec3d_t){w, -h, -(BH-FZ)};

        intvec3d_t N;
        N.z = -FXY;

        // left face
        N.x = -FZ; N.y = 0 ;
        OBJ_rect_push(N, ubl, utl, ltl, lbl);

        // right face
        N.x = FZ; N.y = 0 ;
        OBJ_rect_push(N, ubr, ltr, utr, lbr);

        // top face
        N.y = FZ; N.x = 0 ;
        OBJ_rect_push(N, utl, utr, ltr, ltl);

        // bottom face
        N.y = -FZ; N.y = 0 ;
        OBJ_rect_push(N, ubl, lbr, ubr, lbl);
    }

    // bevel/step foot
    if ((foot.mode == step) || (foot.mode == bevel)) {
        ltl = (intvec3d_t){FXY,         -FXY, -BH};
        ltr = (intvec3d_t){w - FXY,     -FXY, -BH};
        lbl = (intvec3d_t){FXY,     -h + FXY, -BH};
        lbr = (intvec3d_t){w - FXY, -h + FXY, -BH};
    }

    // lower face - prepared XY coordinates differ between foot.mode "no_foot" and "bevel/step"
    OBJ_rect_push(Zn, ltr, lbl, ltl, lbr);

    if (foot.mode == step) { // stepped foot
        // downlooking faces around step rim

        utl = (intvec3d_t){0,  0, -(BH-FZ)};
        utr = (intvec3d_t){w,  0, -(BH-FZ)};
        ubl = (intvec3d_t){0, -h, -(BH-FZ)};
        ubr = (intvec3d_t){w, -h, -(BH-FZ)};


        ltl = (intvec3d_t){FXY,     -FXY,     -(BH-FZ)};
        ltr = (intvec3d_t){w - FXY, -FXY,     -(BH-FZ)};
        lbl = (intvec3d_t){FXY,     -h + FXY, -(BH-FZ)};
        lbr = (intvec3d_t){w - FXY, -h + FXY, -(BH-FZ)};

        // left downward face
        OBJ_rect_push(Zn, ubl, utl, ltl, lbl);

        // right downward face
        OBJ_rect_push(Zn, ubr, ltr, utr, lbr);

        // top downward face
        OBJ_rect_push(Zn, utl, utr, ltr, ltl);

        // bottom downward face
        OBJ_rect_push(Zn, ubl, lbr, ubr, lbl);
    }


    if (tag_bitmap_i32 != NULL)
        free(tag_bitmap_i32);
    tag_bitmap_i32 = NULL;


    return 0;
}


int TypeBitmap::writeOBJ(std::string filename)
{
    int i;
    int w = bm_width;
    int h = bm_height;

    float RS = raster_size.as_mm();
    float LH = layer_height.as_mm();

    if (filename.empty())
    {
        std::cerr << "ERROR: No OBJ file specified." << std::endl;
        return -1;
    }

    std::ofstream obj_out(filename, std::ios::binary);
    if (!obj_out.is_open()) {
        std::cerr << "ERROR: Could not open OBJ file for writing." << std::endl;
        return -1;
    }

    std::cout << "Triangle count is " << triangles.size() << std::endl;

    obj_out << "### OBJ data exported from t3t_pbm2stl:" << std::endl;

    obj_out << std::endl << "# Vertices with coordinates in mm:" << std::endl;
    for (i=1; i<vertices.size(); i++) {
        intvec3d_t vertex = vertices[i];
        int32_t x_0um1 = int32_t(round(vertex.x * RS * 10000));
        int32_t y_0um1 = int32_t(round(vertex.y * RS * 10000));
        int32_t z_0um1 = int32_t(round(vertex.z * LH * 10000));
        float x = vertex.x * RS;
        float y = vertex.y * RS;
        float z = vertex.z * LH;

        obj_out << "v "
                << x << " "
                << y << " "
                << z << " "
//                << (x_0um1 / 10000) << "." << abs(x_0um1 % 10000) << " "    /* doesnt work without leading 0s on fractions*/
//                << (y_0um1 / 10000) << "." << abs(y_0um1 % 10000) << " "
//                << (z_0um1 / 10000) << "." << abs(z_0um1 % 10000) << " "
                << std::endl;
    }

    obj_out << std::endl << "# Triangles by vertex number:" << std::endl;
    for (i=0; i<triangles.size(); i++) {
        mesh_triangle triangle = triangles[i];
        obj_out << "f "
                << triangle.v1 << " "
                << triangle.v2 << " "
                << triangle.v3 << " "
                << std::endl;
    }
    obj_out.close();

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



int TypeBitmap::writeSTL(std::string filename)
{
    int i;
    int w = bm_width;
    int h = bm_height;

    float RS = raster_size.as_mm();
    float LH = layer_height.as_mm();

    if (filename.empty())
    {
        std::cerr << "ERROR: No STL file specified." << std::endl;
        return -1;
    }

    std::ofstream stl_out(filename, std::ios::binary);
    if (!stl_out.is_open()) {
        std::cerr << "ERROR: Could not open STL file for writing." << std::endl;
        return -1;
    }

    std::cout << "Triangle count is " << triangles.size() << std::endl;

    // 80 byte header - content anything but "solid" (would indicated ASCII encoding)
    for (i = 0; i < 80; i++)
        stl_out.put('x');

    uint32_t tri_cnt = triangles.size();
    stl_out.write((const char*)&tri_cnt, 4); // space for number of triangles


    for (i=0; i<triangles.size(); i++) {
        mesh_triangle triangle = triangles[i];
        intvec3d_t N = triangles[i].N;
        intvec3d_t v1 = vertices[triangles[i].v1];
        intvec3d_t v2 = vertices[triangles[i].v2];
        intvec3d_t v3 = vertices[triangles[i].v3];

        stl_tri_t TRI;
        TRI = (stl_tri_t) {float(N.x), float(N.y), float(N.z),
                        v1.x * RS, v1.y * RS, v1.z * LH,
                        v2.x * RS, v2.y * RS, v2.z * LH,
                        v3.x * RS, v3.y * RS, v3.z * LH,
                        0};
        stl_out.write((const char*)&TRI, 50);
    }

    stl_out.close();





/*
    obj_out << "### OBJ data exported from t3t_pbm2stl:" << std::endl;

    obj_out << std::endl << "# Vertices with coordinates in mm:" << std::endl;
    for (i=1; i<vertices.size(); i++) {
        intvec3d_t vertex = vertices[i];
        int32_t x_0um1 = int32_t(round(vertex.x * RS * 10000));
        int32_t y_0um1 = int32_t(round(vertex.y * RS * 10000));
        int32_t z_0um1 = int32_t(round(vertex.z * LH * 10000));
        float x = vertex.x * RS;
        float y = vertex.y * RS;
        float z = vertex.z * LH;

        obj_out << "v "
                << x << " "
                << y << " "
                << z << " "
//                << (x_0um1 / 10000) << "." << abs(x_0um1 % 10000) << " "
//                << (y_0um1 / 10000) << "." << abs(y_0um1 % 10000) << " "
//                << (z_0um1 / 10000) << "." << abs(z_0um1 % 10000) << " "
                << std::endl;
    }

    obj_out << std::endl << "# Triangles by vertex number:" << std::endl;
    for (i=0; i<triangles.size(); i++) {
        mesh_triangle triangle = triangles[i];
        obj_out << "f "
                << triangle.v1 << " "
                << triangle.v2 << " "
                << triangle.v3 << " "
                << std::endl;
    }
    obj_out.close();
*/
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

