#include "TypeBitmap.h"
#include "AppLog.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <boost/format.hpp> 

extern AppLog logger;

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

    uint8_t format;
    if (linebuf=="P1") {
        format = 1;
    }
    else if (linebuf=="P4") {
        format = 4;
    }
    else {
        logger.ERROR() << "Not a valid PBM file!" << std::endl;
        pbm.close();
        return -1;
    }

    // remove comments
    while(pbm.peek()=='#')
        getline(pbm, linebuf);

    if (!(pbm >> bm_width) || (bm_width==0)) {
        logger.ERROR() << "No valid X dimension" << std::endl;
        pbm.close();
        return -1;
    }

    // remove comments
    while(pbm.peek()=='#')
        getline(pbm, linebuf);

    if (!(pbm >> bm_height) || (bm_height==0)) {
        logger.ERROR() << "No valid Y dimension" << std::endl;
        pbm.close();
        return -1;
    }

    bitmap = (uint8_t*)calloc(size = bm_width*bm_height, sizeof(uint8_t));
    if (bitmap == NULL) {
        logger.ERROR() << "Bitmap buffer allocation failed" << std::endl;
        pbm.close();
        return -1;
    }

    uint32_t count = 0;
    uint8_t *runbuf = bitmap;

    // ASCII format (P1)
    if (format==1) {
        char c;

        c = pbm.get();
        while(!pbm.eof()) {
            switch(c) {
                case '#':
                    getline(pbm, linebuf); break;
                case '0':
                case '1':
                    if (count == size) {
                        logger.ERROR() << "More data than specified." << std::endl;
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
                    break;
                default:
                    break;
            }
            c = pbm.get();
        }
    }

    // Binary format (P4)
    if (format==4) {
        uint8_t byte;
        uint32_t x_cnt = 0;
        uint32_t y_cnt = 0;

        byte = (uint8_t)pbm.get(); // whitespace after height
        
        byte = (uint8_t)pbm.get();
        while(!pbm.eof()) {
            if (count >= size) { // file should have ended
                logger.ERROR() << "More data than specified." << std::endl;
                free(bitmap);
                bitmap = NULL;
                pbm.close();
                return -1;
            }

            for (int i=0; i<8; i++) {

                if (byte & 0x80) { // start on MSB
                    *(runbuf++) = 255;
                }
                else {
                    *(runbuf++) = 0;
                }
                byte <<= 1;
                count++;
                x_cnt++;
                if (x_cnt==bm_width) {
                    x_cnt = 0;
                    y_cnt++;
                    break; // don't use rest of bit
                }
            }
            byte = (uint8_t)pbm.get();
        }
    }

    if (count != size) {
        logger.ERROR() << "Less data than specified." << std::endl;
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
        logger.ERROR() << "No bitmap to store." << std::endl;
        return -1;
    }

    std::ofstream pbm(filename);
    if (!pbm.is_open()) {
        logger.ERROR() << "Opening " << filename <<" for writing failed." << std::endl;
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
        logger.ERROR() << "No glyph allocated to paste." << std::endl;
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
        logger.WARNING() << " Glyph didn't fit into bitmap." << std::endl;
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


void TypeBitmap::push_triangles(intvec3d_t N, intvec3d_t v1, intvec3d_t v2, intvec3d_t v3, intvec3d_t v4)
{
    int n = 4; // quadrilateral (==2 triangles) by default
    if (INT32_MAX == v4.z) // only 3 points specified
        n=3;

    float twoPi = 8*atan(1);
    float halfPi = 2*atan(1);

    float center_x, center_y;

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

//     full two-step X-Y for the record
//        pos3d_t P1 = { 1, 1, 0 };
//        Nb.x =  P1.x*cosrY + P1.z*sinrY;
//        Nb.y =  P1.y;
//        Nb.z = -P1.x*sinrY + P1.z*cosrY;
//        P1.x = Nb.x;
//        P1.y = Nb.y*cosrX - Nb.z*sinrX;
//        P1.z = Nb.y*sinrX + Nb.z*cosrX;
//        std::cout << "P1 after Y-X rot:  x=" << P1.x << ", y=" << P1.y << ", z=" << P1.z << std::endl;


    for (int i=0; i<n; i++) {
        float x = float(proj[i].vert3d->x);
        float y = float(proj[i].vert3d->y);
        float z = float(proj[i].vert3d->z);
        proj[i].px = x*cosrY + z*sinrY;
        proj[i].py = y*cosrX - (-x*sinrY + z*cosrY)*sinrX;
    }

    if (4==n) { // quadrilateral
        center_x = (proj[0].px + proj[1].px + proj[2].px + proj[3].px) / 4;
        center_y = (proj[0].py + proj[1].py + proj[2].py + proj[3].py) / 4;
    }
    else { // single triangle
        center_x = (proj[0].px + proj[1].px + proj[2].px) / 3;
        center_y = (proj[0].py + proj[1].py + proj[2].py) / 3;
    }

    for (int i=0; i<n; i++) {
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

    // bubble sort
    prj2d_t swap;

    for (int i=0; i<(n-1); i++) {
       for (int j=0; j<(n-i-1); j++) {
            if (proj[j].angle > proj[j+1].angle) {
                swap = proj[j]; // TODO optimize: don't swap px, py
                proj[j] = proj[j+1];
                proj[j+1] = swap;
            }
       }
    }

    mesh_triangle TRI;

    TRI.N = N;
    TRI.v1 = find_or_add_vertex(*(proj[0].vert3d));
    TRI.v2 = find_or_add_vertex(*(proj[1].vert3d));
    TRI.v3 = find_or_add_vertex(*(proj[2].vert3d));
    triangles.push_back(TRI);

    if (4==n) { // quadrilateral, so there is a 2nd triangle
        TRI.N = N;
        TRI.v1 = find_or_add_vertex(*(proj[0].vert3d));
        TRI.v2 = find_or_add_vertex(*(proj[2].vert3d));
        TRI.v3 = find_or_add_vertex(*(proj[3].vert3d));
        triangles.push_back(TRI);
    }
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


void TypeBitmap::fill_rectangle(int32_t *bm32, STLrect rect)
{
    int32_t value = rect.tag;
    int32_t xmax = rect.width;
    int32_t ymax = rect.height;

    bm32 += (rect.top*bm_width + rect.left); // start position

    for (int y=0; y<ymax; y++) {
        for (int x=0; x<xmax; x++) {
            *bm32++ = value;
        }
        bm32 -= rect.width; // line start
        bm32 += bm_width; // next line
    }
}


int TypeBitmap::find_rectangles(void)
{
    int x, y;
    int i, j, k;

    int w = bm_width;
    int h = bm_height;
    uint8_t *buf8 = (uint8_t*)bitmap;
    int32_t *buf32;

    if (!loaded)
    {
        logger.ERROR() << "No Bitmap loaded." << std::endl;
        return -1;
    }

    glyph_rects.clear();
    body_rects.clear();

    tag_bitmap_i32 = (int32_t*)calloc(w*h, sizeof(int32_t));
    if (tag_bitmap_i32 == NULL) {
        logger.ERROR() << "Could not allocate tag bitmap." << std::endl;
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

        valrect.width = valrect.right - valrect.left + 1;
        valrect.height = valrect.bottom - valrect.top + 1;

        //if (expanded) {
        if ((valrect.width!=1) && (valrect.height!=1)) {
           

            buf32 = tag_bitmap_i32;
            buf32 += (valrect.top*w);
            for (j=valrect.top; j<=valrect.bottom; j++) {
                for (k=valrect.left; k<=valrect.right; k++) {
                    buf32[k] = valrect.tag;
                }
                buf32 += w;
            }
            buf32 = tag_bitmap_i32;

            if (val==-1) {
                body_rects.push_back(valrect);
            }
            if (val==+1) {
                glyph_rects.push_back(valrect);
            }


            tag_cnt++;
        }
    }

    return 0;
}


int TypeBitmap::generateMesh(reduced_foot foot, std::vector<nick> &nicks, float UVstretchXY, float UVstretchZ)
{
    int x, y;
    int i, j, k;
    int w = bm_width;
    int h = bm_height;

    vertices.clear();
    intvec3d_t vec3dbuf = (intvec3d_t) {INT32_MIN, INT32_MIN, INT32_MIN};
    vertices.push_back(vec3dbuf); // vertex #0, as OBJ files start indexing at #1

    triangles.clear();


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
    else if (foot.mode == supports) {
        FZ = int32_t( round( (1.50 * UVstretchZ)/LH ) ); // TODO: fixed at 1.25mm for now, overriding YAML
        FXY = 0; // none
    }
    else {
        FZ = int32_t( round( (foot.Z.as_mm() * UVstretchZ)/LH ) );
        FXY = int32_t( round( foot.XY.as_mm()/RS  ) );
    }

    int32_t PFH = int32_t( round( (UVstretchZ*(foot.pyramid_foot_height.as_mm()))/LH ) );

    if (find_rectangles() <0)
        return -1;


    if (!loaded)
    {
        logger.ERROR() << "No Bitmap loaded." << std::endl;
        return -1;
    }

    int32_t *buf32 = tag_bitmap_i32;

    // normal vectors: X, Y, Z, positive, negative
    intvec3d_t Xp = (intvec3d_t){ 1,  0,  0};  intvec3d_t Xn = (intvec3d_t){-1,  0,  0};
    intvec3d_t Yp = (intvec3d_t){ 0,  1,  0};  intvec3d_t Yn = (intvec3d_t){ 0, -1,  0};
    intvec3d_t Zp = (intvec3d_t){ 0,  0,  1};  intvec3d_t Zn = (intvec3d_t){ 0,  0, -1};



    // cube corners: upper/lower;top/botton;left/right
    intvec3d_t utl, utr, ubl, ubr, ltl, ltr, lbl, lbr;


    // LARGE RECTS (min 2x2)
    // body top surface
    for (i = 0; i < body_rects.size(); i++) {
        STLrect R = body_rects[i];

        std::vector<intvec2d_t> outsides;
        int32_t last_tag, current_tag;
        bool Rneg = (R.tag < 0);

        // top side
        outsides.push_back((intvec2d_t){R.left,R.top}); //top left corner, always needed
        if (0==R.top) { // edge
            outsides.push_back((intvec2d_t){R.right+1,R.top});
        }
        else { // iterate
            int32_t top_x = R.left;
            int32_t top_y = R.top - 1; // line above rect
            last_tag = buf32[top_y * w + top_x];
            top_x++;
            while (top_x <= R.right) {
                current_tag = buf32[top_y * w + top_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){top_x,R.top});
                }
                last_tag = current_tag;
                top_x++;
            }
        }

        // right side
        outsides.push_back((intvec2d_t){R.right+1,R.top}); //top right corner, always needed
        if (w-1==R.right) { // edge
            outsides.push_back((intvec2d_t){R.right+1,R.bottom+1});
        }
        else { // iterate
            int32_t right_x = R.right + 1;  // line right of rect
            int32_t right_y = R.top;
            last_tag = buf32[right_y * w + right_x];
            right_y++;
            while (right_y <= R.bottom) {
                current_tag = buf32[right_y * w + right_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){R.right+1,right_y});
                }
                last_tag = current_tag;
                right_y++;
            }
        }

        // bottom side
        outsides.push_back((intvec2d_t){R.right+1,R.bottom+1}); //bottom right corner, always needed
        if (h-1==R.bottom) { // edge
            outsides.push_back((intvec2d_t){R.left,R.bottom+1});
        }
        else { // iterate
            int32_t bottom_x = R.right;
            int32_t bottom_y = R.bottom + 1; // line below rect
            last_tag = buf32[bottom_y * w + bottom_x];
            bottom_x--;
            while (bottom_x >= R.left) {
                current_tag = buf32[bottom_y * w + bottom_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){bottom_x+1,R.bottom+1});
                }
                last_tag = current_tag;
                bottom_x--;
            }
        }

        // left side
        outsides.push_back((intvec2d_t){R.left,R.bottom+1}); //bottom left corner, always needed
        if (0==R.left) { // edge
            outsides.push_back((intvec2d_t){R.left,R.top});
        }
        else { // iterate
            int32_t left_x = R.left - 1;  // line left of rect
            int32_t left_y = R.bottom;
            last_tag = buf32[left_y * w + left_x];
            left_y--;
            while (left_y >= R.top) {
                current_tag = buf32[left_y * w + left_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){R.left,left_y+1});
                }
                last_tag = current_tag;
                left_y--;
            }
        }

        intvec3d_t center, corner1, corner2;

        center.x = R.left + (R.width / 2);
        center.y = -(R.top + (R.height / 2));
        center.z = 0; // body top surface

        corner1.x = outsides[0].x;
        corner1.y = -outsides[0].y;
        corner1.z = 0;

        corner2.x = outsides[outsides.size()-1].x;
        corner2.y = -outsides[outsides.size()-1].y;
        corner2.z = 0;

        push_triangles(Zp, center, corner1, corner2);

        for (j=1; j<outsides.size(); j++) {
            corner1.x = outsides[j-1].x;
            corner1.y = -outsides[j-1].y;
            corner2.x = outsides[j].x;
            corner2.y = -outsides[j].y;
            push_triangles(Zp, center, corner1, corner2);
        }
    }

    // glyph top surface
    for (i = 0; i < glyph_rects.size(); i++) {
        STLrect R = glyph_rects[i];

        std::vector<intvec2d_t> outsides;
        int32_t last_tag, current_tag;
        bool Rneg = (R.tag < 0);

        // top side
        outsides.push_back((intvec2d_t){R.left,R.top}); //top left corner, always needed
        if (0==R.top) { // edge
            outsides.push_back((intvec2d_t){R.right+1,R.top});
        }
        else { // iterate
            int32_t top_x = R.left;
            int32_t top_y = R.top - 1; // line above rect
            last_tag = buf32[top_y * w + top_x];
            top_x++;
            while (top_x <= R.right) {
                current_tag = buf32[top_y * w + top_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){top_x,R.top});
                }
                last_tag = current_tag;
                top_x++;
            }
        }

        // right side
        outsides.push_back((intvec2d_t){R.right+1,R.top}); //top right corner, always needed
        if (w-1==R.right) { // edge
            outsides.push_back((intvec2d_t){R.right+1,R.bottom+1});
        }
        else { // iterate
            int32_t right_x = R.right + 1;  // line right of rect
            int32_t right_y = R.top;
            last_tag = buf32[right_y * w + right_x];
            right_y++;
            while (right_y <= R.bottom) {
                current_tag = buf32[right_y * w + right_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){R.right+1,right_y});
                }
                last_tag = current_tag;
                right_y++;
            }
        }

        // bottom side
        outsides.push_back((intvec2d_t){R.right+1,R.bottom+1}); //bottom right corner, always needed
        if (h-1==R.bottom) { // edge
            outsides.push_back((intvec2d_t){R.left,R.bottom+1});
        }
        else { // iterate
            int32_t bottom_x = R.right;
            int32_t bottom_y = R.bottom + 1; // line below rect
            last_tag = buf32[bottom_y * w + bottom_x];
            bottom_x--;
            while (bottom_x >= R.left) {
                current_tag = buf32[bottom_y * w + bottom_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){bottom_x+1,R.bottom+1});
                }
                last_tag = current_tag;
                bottom_x--;
            }
        }

        // left side
        outsides.push_back((intvec2d_t){R.left,R.bottom+1}); //bottom left corner, always needed
        if (0==R.left) { // edge
            outsides.push_back((intvec2d_t){R.left,R.top});
        }
        else { // iterate
            int32_t left_x = R.left - 1;  // line left of rect
            int32_t left_y = R.bottom;
            last_tag = buf32[left_y * w + left_x];
            left_y--;
            while (left_y >= R.top) {
                current_tag = buf32[left_y * w + left_x];
                if ((1==abs(current_tag)) || (current_tag!=last_tag) || (Rneg != (current_tag < 0))) { // single field, vertical wall or changed
                    outsides.push_back((intvec2d_t){R.left,left_y+1});
                }
                last_tag = current_tag;
                left_y--;
            }
        }

        intvec3d_t center, corner1, corner2;

        center.x = R.left + (R.width / 2);
        center.y = -(R.top + (R.height / 2));
        center.z = DOD; // glyph top

        corner1.x = outsides[0].x;
        corner1.y = -outsides[0].y;
        corner1.z = DOD;

        corner2.x = outsides[outsides.size()-1].x;
        corner2.y = -outsides[outsides.size()-1].y;
        corner2.z = DOD;

        push_triangles(Zp, center, corner1, corner2);

        for (j=1; j<outsides.size(); j++) {
            corner1.x = outsides[j-1].x;
            corner1.y = -outsides[j-1].y;
            corner2.x = outsides[j].x;
            corner2.y = -outsides[j].y;
            push_triangles(Zp, center, corner1, corner2);
        }

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
                push_triangles(Zp, utr, utl, ubl, ubr);
            }

            // single pixel upper faces (body / no glyph)
            if (buf32[y * w + x] == -1) {
                // lower faces become upper faces of body for 0
                push_triangles(Zp, ltr, lbl, ltl, lbr);
            }

            // side walls of glyph
            if (buf32[y * w + x] > 0) {
                
                // left face
                if ((x == 0) || (buf32[((y)*w) + (x - 1)] < 0)) {
                    push_triangles(Xn, ubl, utl, ltl, lbl);
                }

                // right face
                if ((x == (bm_width - 1)) || (buf32[((y)*w) + (x + 1)] < 0)) {
                    push_triangles(Xp, ubr, ltr, utr, lbr);
                }

                // top face
                if ((y == 0) || (buf32[((y - 1) * w) + (x)] < 0)) {
                    push_triangles(Yp, utl, utr, ltr, ltl);
                }

                // bottom face
                if ((y == (bm_height - 1)) || (buf32[((y + 1) * w) + (x)] < 0)) {
                    push_triangles(Yn, ubl, lbr, ubr, lbl);
                }
            }
        }
    }

    int32_t BLC = 0; // body layer count

    // BODY UPPER STRIP - constant 2mm for now
    int32_t US = int32_t(round(2 * UVstretchZ) / LH);

    utl = (intvec3d_t){0,  0, 0};
    utr = (intvec3d_t){w,  0, 0};
    ubl = (intvec3d_t){0, -h, 0};
    ubr = (intvec3d_t){w, -h, 0};

    ltl = (intvec3d_t){0,  0, -US};
    ltr = (intvec3d_t){w,  0, -US};
    lbl = (intvec3d_t){0, -h, -US};
    lbr = (intvec3d_t){w, -h, -US};

    intvec3d_t top_center    = (intvec3d_t){w/2,   0, -(US/2)};
    intvec3d_t bottom_center = (intvec3d_t){w/2,  -h, -(US/2)};
    intvec3d_t left_center   = (intvec3d_t){0,  -h/2, -(US/2)};
    intvec3d_t right_center  = (intvec3d_t){w,  -h/2, -(US/2)};

    std::vector<intvec3d_t> top_edge, right_edge, bottom_edge, left_edge; // upper surface points to be connected
    int32_t last_tag, current_tag;

    // upper strip - top edge
    top_edge.push_back(utr);
    top_edge.push_back(ltr);
    top_edge.push_back(ltl);
    top_edge.push_back(utl);
    buf32 = tag_bitmap_i32;
    last_tag = *buf32; // upper left pixel
    buf32++;
    for (int x=1; x<bm_width; x++) {
        current_tag = *buf32;
        if ((current_tag>0) || (current_tag!=last_tag)) {
            top_edge.push_back((intvec3d_t){x, 0, 0});
        }
        last_tag = current_tag;
        buf32++;
    }

    push_triangles(Yp, top_center, top_edge[0], top_edge[top_edge.size()-1]);

    for (j=1; j<top_edge.size(); j++) {
        push_triangles(Yp, top_center, top_edge[j-1], top_edge[j]);
    }

    // upper strip - right edge
    right_edge.push_back(ubr);
    right_edge.push_back(lbr);
    right_edge.push_back(ltr);
    right_edge.push_back(utr);
    buf32 = tag_bitmap_i32;
    buf32 += (bm_width - 1);
    last_tag = *buf32; // upper right pixel
    buf32 += bm_width;
    for (int y=1; y<bm_height; y++) {
        current_tag = *buf32;
        if ((current_tag>0) || (current_tag!=last_tag)) {
            right_edge.push_back((intvec3d_t){w, -y, 0});
        }
        last_tag = current_tag;
        buf32 += bm_width;
    }

    push_triangles(Xp, right_center, right_edge[0], right_edge[right_edge.size()-1]);

    for (j=1; j<right_edge.size(); j++) {
        push_triangles(Xp, right_center, right_edge[j-1], right_edge[j]);
    }

    // upper strip - bottom edge
    bottom_edge.push_back(ubr);
    bottom_edge.push_back(lbr);
    bottom_edge.push_back(lbl);
    bottom_edge.push_back(ubl);
    buf32 = tag_bitmap_i32;
    buf32 += (bm_width*(bm_height-1));
    last_tag = *buf32; // lower left pixel
    buf32++;
    for (int x=1; x<bm_width; x++) {
        current_tag = *buf32;
        if ((current_tag>0) || (current_tag!=last_tag)) {
            bottom_edge.push_back((intvec3d_t){x, -h, 0});
        }
        last_tag = current_tag;
        buf32++;
    }

    push_triangles(Yn, bottom_center, bottom_edge[0], bottom_edge[bottom_edge.size()-1]);

    for (j=1; j<bottom_edge.size(); j++) {
        push_triangles(Yn, bottom_center, bottom_edge[j-1], bottom_edge[j]);
    }

    // upper strip - left edge
    left_edge.push_back(ubl);
    left_edge.push_back(lbl);
    left_edge.push_back(ltl);
    left_edge.push_back(utl);
    buf32 = tag_bitmap_i32;
    last_tag = *buf32; // top left pixel
    buf32 += bm_width;
    for (int y=1; y<bm_height; y++) {
        current_tag = *buf32;
        if ((current_tag>0) || (current_tag!=last_tag)) {
            left_edge.push_back((intvec3d_t){0, -y, 0});
        }
        last_tag = current_tag;
        buf32 += bm_width;
    }

    push_triangles(Xn, left_center, left_edge[0], left_edge[left_edge.size()-1]);

    for (j=1; j<left_edge.size(); j++) {
        push_triangles(Xn, left_center, left_edge[j-1], left_edge[j]);
    }

    BLC += US;


    // NICK LAYERS
    for (int i = 0; i< nicks.size(); i++) {

        // NICK HEIGHT
        int32_t NH = int32_t(round(nicks[i].z.as_mm() * UVstretchZ) / LH);

        // TRIANGLE DEPTH - half of height
        int32_t TD = int32_t(round((nicks[i].z.as_mm()/2) * UVstretchXY) / RS);

        // RECT DEPTH
        int32_t RD = int32_t(round(nicks[i].y.as_mm() * UVstretchXY) / RS);

        if (nicks[i].type == rect) {
            // RECT SEGMENT
            utl = (intvec3d_t){0,  0, -BLC};
            utr = (intvec3d_t){w,  0, -BLC};
            ubl = (intvec3d_t){0, -(h-RD), -BLC};
            ubr = (intvec3d_t){w, -(h-RD), -BLC};

            ltl = (intvec3d_t){0,  0, -(BLC+NH)};
            ltr = (intvec3d_t){w,  0, -(BLC+NH)};
            lbl = (intvec3d_t){0, -(h-RD), -(BLC+NH)};
            lbr = (intvec3d_t){w, -(h-RD), -(BLC+NH)};

            push_triangles(Xn, ubl, utl, ltl, lbl); // left face
            push_triangles(Xp, ubr, ltr, utr, lbr); // right face
            push_triangles(Yp, utl, utr, ltr, ltl); // top face
            push_triangles(Yn, ubl, lbr, ubr, lbl); // bottom face
            
            utl = (intvec3d_t){0, -(h-RD), -BLC};
            utr = (intvec3d_t){w, -(h-RD), -BLC};
            ubl = (intvec3d_t){0, -h, -BLC};
            ubr = (intvec3d_t){w, -h, -BLC};

            ltl = (intvec3d_t){0, -(h-RD), -(BLC+NH)};
            ltr = (intvec3d_t){w, -(h-RD), -(BLC+NH)};
            lbl = (intvec3d_t){0, -h, -(BLC+NH)};
            lbr = (intvec3d_t){w, -h, -(BLC+NH)};

            push_triangles(Zn, utl, utr, ubl, ubr); // downward-looking upper face of nick
            push_triangles(Zp, ltl, ltr, lbl, lbr); // upward-looking lower face of nick
        }
        else if (nicks[i].type == triangle) {
            // TRIANGLE SEGMENT
            utl = (intvec3d_t){0,  0, -BLC};
            utr = (intvec3d_t){w,  0, -BLC};
            ubl = (intvec3d_t){0, -h, -BLC};
            ubr = (intvec3d_t){w, -h, -BLC};

            ltl = (intvec3d_t){0,  0, -(BLC+(NH/2))};
            ltr = (intvec3d_t){w,  0, -(BLC+(NH/2))};
            lbl = (intvec3d_t){0, -(h-TD), -(BLC+(NH/2))};
            lbr = (intvec3d_t){w, -(h-TD), -(BLC+(NH/2))};

            intvec3d_t YnZn = (intvec3d_t){ 0,  -1,  -1};
            intvec3d_t YnZp = (intvec3d_t){ 0,  -1,   1};

            push_triangles(Xn, ubl, utl, ltl, lbl); // left face
            push_triangles(Xp, ubr, ltr, utr, lbr); // right face
            push_triangles(Yp, utl, utr, ltr, ltl); // top face
            push_triangles(YnZn, ubl, lbr, ubr, lbl); // bottom face
            
            utl = (intvec3d_t){0,  0, -(BLC+(NH/2))};
            utr = (intvec3d_t){w,  0, -(BLC+(NH/2))};
            ubl = (intvec3d_t){0, -(h-TD), -(BLC+(NH/2))};
            ubr = (intvec3d_t){w, -(h-TD), -(BLC+(NH/2))};

            ltl = (intvec3d_t){0,  0, -(BLC+NH)};
            ltr = (intvec3d_t){w,  0, -(BLC+NH)};
            lbl = (intvec3d_t){0, -h, -(BLC+NH)};
            lbr = (intvec3d_t){w, -h, -(BLC+NH)};

            push_triangles(Xn, ubl, utl, ltl, lbl); // left face
            push_triangles(Xp, ubr, ltr, utr, lbr); // right face
            push_triangles(Yp, utl, utr, ltr, ltl); // top face
            push_triangles(YnZp, ubl, lbr, ubr, lbl); // bottom face
        }
        else if (nicks[i].type == circle) {

            float twoPi = 8*atan(1);
            float Pi = 4*atan(1);
            float halfPi = 2*atan(1);

            const int32_t circle_segs = 10;

            for (int j=0; j< circle_segs; j++) {

                // TODO make special cases for first and last to avoid rounding-error integer shift
                float start_angle = j*(Pi/circle_segs);
                float end_angle   = (j+1)*(Pi/circle_segs);

                float start_zf = -((cos(start_angle)-1)/2) * nicks[i].z.as_mm() * UVstretchZ;  // 0.0..-1.0
                float end_zf   = -((cos(end_angle  )-1)/2) * nicks[i].z.as_mm() * UVstretchZ;  // 0.0..-1.0
                int32_t start_z = int32_t(round(start_zf/LH));
                int32_t end_z   = int32_t(round(end_zf/LH));
                if (j==0)
                    start_z==0;
                if (start_z > NH)
                    start_z = NH;
                if (end_z > NH)
                    end_z = NH;

                float start_yf = sin(start_angle) * (nicks[i].z.as_mm()/2) * UVstretchXY;
                float end_yf   = sin(end_angle) * (nicks[i].z.as_mm()/2) * UVstretchXY;
                int32_t start_y = int32_t(round(start_yf/RS));
                int32_t end_y   = int32_t(round(end_yf/RS));

                // MAKE N
                intvec3d_t Nang = (intvec3d_t) { 0,  -(end_z-start_z), -(end_y-start_y)};

                utl = (intvec3d_t){0,  0, -(BLC+start_z)};
                utr = (intvec3d_t){w,  0, -(BLC+start_z)};
                ubl = (intvec3d_t){0, -(h-start_y), -(BLC+start_z)};
                ubr = (intvec3d_t){w, -(h-start_y), -(BLC+start_z)};

                ltl = (intvec3d_t){0,  0, -(BLC+end_z)};
                ltr = (intvec3d_t){w,  0, -(BLC+end_z)};
                lbl = (intvec3d_t){0, -(h-end_y), -(BLC+end_z)};
                lbr = (intvec3d_t){w, -(h-end_y), -(BLC+end_z)};

                push_triangles(Xn, ubl, utl, ltl, lbl); // left face
                push_triangles(Xp, ubr, ltr, utr, lbr); // right face
                push_triangles(Yp, utl, utr, ltr, ltl); // top face
                push_triangles(Nang, ubl, lbr, ubr, lbl); // bottom face
            }
        }
        else {
            // FLAT SEGMENT - TODO TODO OTHER SEGMENTS
            utl = (intvec3d_t){0,  0, -BLC};
            utr = (intvec3d_t){w,  0, -BLC};
            ubl = (intvec3d_t){0, -h, -BLC};
            ubr = (intvec3d_t){w, -h, -BLC};

            ltl = (intvec3d_t){0,  0, -(BLC+NH)};
            ltr = (intvec3d_t){w,  0, -(BLC+NH)};
            lbl = (intvec3d_t){0, -h, -(BLC+NH)};
            lbr = (intvec3d_t){w, -h, -(BLC+NH)};

            push_triangles(Xn, ubl, utl, ltl, lbl); // left face
            push_triangles(Xp, ubr, ltr, utr, lbr); // right face
            push_triangles(Yp, utl, utr, ltr, ltl); // top face
            push_triangles(Yn, ubl, lbr, ubr, lbl); // bottom face

        }

        BLC += NH;
    }



    if (foot.mode != pyramids) {

        // LOWER BODY STRIP (above reduced foot, if exists)  TODO - make different one for supports-foot
        utl = (intvec3d_t){0,  0, -BLC};
        utr = (intvec3d_t){w,  0, -BLC};
        ubl = (intvec3d_t){0, -h, -BLC};
        ubr = (intvec3d_t){w, -h, -BLC};
        ltl = (intvec3d_t){0,  0, -(BH-FZ)};
        ltr = (intvec3d_t){w,  0, -(BH-FZ)};
        lbl = (intvec3d_t){0, -h, -(BH-FZ)};
        lbr = (intvec3d_t){w, -h, -(BH-FZ)};


    }

    if ((foot.mode != supports) && (foot.mode != pyramids)) { // all other feet, not pyramids, not supports

        // TODO: find out where these corners were defined
        push_triangles(Xn, ubl, utl, ltl, lbl); // left face
        push_triangles(Xp, ubr, ltr, utr, lbr); // right face

        // LOWER BODY STRIP (above reduced foot, if exists)  TODO - make different one for supports-foot
        utl = (intvec3d_t){0,  0, -BLC};
        utr = (intvec3d_t){w,  0, -BLC};
        ubl = (intvec3d_t){0, -h, -BLC};
        ubr = (intvec3d_t){w, -h, -BLC};
        ltl = (intvec3d_t){0,  0, -(BH-FZ)};
        ltr = (intvec3d_t){w,  0, -(BH-FZ)};
        lbl = (intvec3d_t){0, -h, -(BH-FZ)};
        lbr = (intvec3d_t){w, -h, -(BH-FZ)};

        push_triangles(Yp, utl, utr, ltr, ltl); // top face
        push_triangles(Yn, ubl, lbr, ubr, lbl); // bottom face
    }




    // PREPARE SUPPORTS + FOOT + LOWER STRIP
    // TODO Lots of constants - consider what should be configurable?

        // Y-direction dimensions
        int32_t BS = bm_height;
        float body_size = (BS * raster_size.as_mm()) / UVstretchXY;
        const float side_support_width = 0.25; // mm
        const float half_support_width = 0.75; // mm
        const float min_support_intervalY = 5.0; // mm
        const float hollow_triangle_width = 1.25;//1.00; // mm
        // unstretched calculations
        int32_t support_intervalsY = int((body_size-2*side_support_width)/min_support_intervalY); // rounded down
        float support_interval = ((body_size-2*side_support_width)/support_intervalsY);
        float interval_gap = support_interval - (2 * hollow_triangle_width);
        float triangle_support_gap = hollow_triangle_width - half_support_width;
//        logger.INFO() << "#support intervals: " << support_intervalsY << std::endl
//                  << "      #support intervals(mm): " << support_interval << std::endl
//                  << "      #intervals gap(mm): " << interval_gap << std::endl
//                  << "      triangle_support_gap(mm): " << triangle_support_gap << std::endl;

        // stretch + discretize
        int32_t SI  = round((support_interval*UVstretchXY) / raster_size.as_mm());
        int32_t SSW = round((side_support_width*UVstretchXY) / raster_size.as_mm());
        int32_t HTW = round((hollow_triangle_width*UVstretchXY) / raster_size.as_mm());
        int32_t IG  = SI-2*HTW;//round((interval_gap*UVstretchXY) / raster_size.as_mm());
        int32_t HSW = round((half_support_width*UVstretchXY) / raster_size.as_mm());
        int32_t TSG = HTW-HSW;//round((triangle_support_gap*UVstretchXY) / raster_size.as_mm());


        // Z-direction dimensions
        const float hollow_triangle_height = 1.25;//1.00; // mm
        const float solid_foot_height = 0.25; // mm
        const float support_gap_height = 3.0; // mm
        const float support_base_height = 1.0; // mm
        // stretch + discretize
        int32_t HTH = round((hollow_triangle_height*UVstretchZ) / layer_height.as_mm());
        int32_t SFH = round((solid_foot_height*UVstretchZ) / layer_height.as_mm());
        int32_t SGH = round((support_gap_height*UVstretchZ) / layer_height.as_mm());
        int32_t SBH = round((support_base_height*UVstretchZ) / layer_height.as_mm());
        // resulting foot heights
        int32_t FH1 = -(BH-FZ);
        int32_t FH2 = FH1-HTH;
        int32_t FH3 = FH2-SFH;
        int32_t FH4 = FH3-SGH;
        int32_t FH5 = FH4-SBH;


        // X-direction dimensions
        int32_t SW = bm_width;
        const float x_support_gap = 1.5; // mm
        const float min_support_intervalX = 4.5; // mm - includes gap (half on each side of gap)??
        float set_width_size = (SW * raster_size.as_mm()) / UVstretchXY;

        int32_t support_intervalsX = int(set_width_size/min_support_intervalX); // rounded down
        if (support_intervalsX < 1)
            support_intervalsX = 1;
        float support_widthX = (set_width_size - ((support_intervalsX-1)*x_support_gap)) / support_intervalsX;

        std::vector<int32_t> support_segments_x1;
        std::vector<int32_t> support_segments_x2;

        for (int i=0; i<support_intervalsX; i++) {

            float f_x1 = i * (x_support_gap + support_widthX);
            float f_x2 = f_x1 + support_widthX;

            int32_t x1 = round((f_x1 * UVstretchXY)/ raster_size.as_mm());
            int32_t x2 = round((f_x2 * UVstretchXY)/ raster_size.as_mm());

            support_segments_x1.push_back(x1);
            support_segments_x2.push_back(x2);
        }
        // fix rounding errors
        support_segments_x1[0] = 0;
        support_segments_x2[support_intervalsX-1] = SW;

    // generate mesh connection points to support foot structure
    std::vector<int32_t> support_foot_verticesY;

    int32_t Ycnt = 0;
    support_foot_verticesY.push_back(Ycnt);
    Ycnt += SSW; // top support
    support_foot_verticesY.push_back(Ycnt);

    // interval segments of triangle gap layer
    for (int i=0; i< support_intervalsY; i++) {

        support_foot_verticesY.push_back(Ycnt);
        support_foot_verticesY.push_back(Ycnt+HTW);
        support_foot_verticesY.push_back(Ycnt+HTW+IG);
        support_foot_verticesY.push_back(Ycnt+HTW+IG+HTW);

        Ycnt += (HTW + IG + HTW);
    }
    support_foot_verticesY.push_back(BS);


    if (foot.mode == supports) {

        //side center
        int32_t cz = -(BLC+(BH-FZ))/2;
        int32_t cy = -BS/2;

        // left face
        intvec3d_t center_left = {0, cy, cz};
        intvec3d_t l1 = {0, 0, -(BH-FZ)};
        intvec3d_t l2 = {0, 0, -(BH-FZ)};

        for (int i=0; i< (support_foot_verticesY.size()-1); i++) {
            l1.y = -support_foot_verticesY[i];
            l2.y = -support_foot_verticesY[i+1];
            push_triangles(Xn, l1, l2, center_left);
        }
        push_triangles(Xn, utl, ubl, center_left);
        push_triangles(Xn, utl, ltl, center_left);
        push_triangles(Xn, ubl, lbl, center_left);

        // right face
        intvec3d_t center_right = {w, cy, cz};
        intvec3d_t r1 = {w, 0, -(BH-FZ)};
        intvec3d_t r2 = {w, 0, -(BH-FZ)};

        for (int i=0; i< (support_foot_verticesY.size()-1); i++) {
            r1.y = -support_foot_verticesY[i];
            r2.y = -support_foot_verticesY[i+1];
            push_triangles(Xp, r1, r2, center_right);
        }
        push_triangles(Xp, utr, ubr, center_right);
        push_triangles(Xp, utr, ltr, center_right);
        push_triangles(Xp, ubr, lbr, center_right);


        // Top/Bottom faces above supports
        intvec3d_t top_center = {w/2, 0 , cz}; // TODO FIX z
        intvec3d_t t1 = {0, 0, -(BH-FZ)};
        intvec3d_t t2 = {0, 0, -(BH-FZ)};

        intvec3d_t bottom_center = {w/2, -h , cz}; // TODO FIX z
        intvec3d_t b1 = {0, -h, -(BH-FZ)};
        intvec3d_t b2 = {0, -h, -(BH-FZ)};

        std::vector<int32_t> all_supportX_points;
        all_supportX_points = support_segments_x1;
        for (int j=0; j<support_segments_x2.size(); j++)
            all_supportX_points.push_back(support_segments_x2[j]);
        std::sort(all_supportX_points.begin(),all_supportX_points.end());

        for (int i=0; i< (all_supportX_points.size()-1); i++) {
            t1.x = all_supportX_points[i];
            t2.x = all_supportX_points[i+1];
            push_triangles(Yp, t1, t2, top_center);
            b1.x = all_supportX_points[i];
            b2.x = all_supportX_points[i+1];
            push_triangles(Yn, b1, b2, bottom_center);
        }
        push_triangles(Yp, utl, utr, top_center);
        push_triangles(Yp, utl, ltl, top_center);
        push_triangles(Yp, utr, ltr, top_center);
        push_triangles(Yn, ubl, ubr, bottom_center);
        push_triangles(Yn, ubl, lbl, bottom_center);
        push_triangles(Yn, ubr, lbr, bottom_center);


        // down-facing between split supports
        for (int j=0; j<(support_intervalsX-1); j++) {

            int32_t left_edge = support_segments_x2[j];
            int32_t right_edge = support_segments_x1[j+1];

            intvec3d_t center = {(left_edge+right_edge)/2, -h/2 , -(BH-FZ)};
            intvec3d_t l1 = {left_edge, 0, -(BH-FZ)};
            intvec3d_t l2 = {left_edge, -h, -(BH-FZ)};
            intvec3d_t r1 = {right_edge, 0, -(BH-FZ)};
            intvec3d_t r2 = {right_edge, -h, -(BH-FZ)};
            push_triangles(Zn, l1, r1, center);
            push_triangles(Zn, l2, r2, center);

            for (int i=0; i< (support_foot_verticesY.size()-1); i++) {
                l1.y = -support_foot_verticesY[i];
                l2.y = -support_foot_verticesY[i+1];
                push_triangles(Zn, l1, l2, center);
                r1.y = -support_foot_verticesY[i];
                r2.y = -support_foot_verticesY[i+1];
                push_triangles(Zn, r1, r2, center);
            }

        }

        // layers in Y direction, X-segmented
        for (int j=0; j<support_intervalsX; j++) {

            int seg_x1 =  support_segments_x1[j];//(w/8)*(j*2);
            int seg_x2 =  support_segments_x2[j];//(w/8)*(j*2+1);

            int32_t TBC = 0; // top-to-buttom count
            intvec3d_t YnZn = (intvec3d_t){ 0,  -1,  -1};
            intvec3d_t YpZn = (intvec3d_t){ 0,   1,  -1};

            // top side support strip
            utl = (intvec3d_t){seg_x1,  0, FH1}; utr = (intvec3d_t){seg_x2,  0, FH1};
            ubl = (intvec3d_t){seg_x1, -SSW, FH1}; ubr = (intvec3d_t){seg_x2, -SSW, FH1};
            ltl = (intvec3d_t){seg_x1,  0, FH2}; ltr = (intvec3d_t){seg_x2,  0, FH2};
            lbl = (intvec3d_t){seg_x1, -SSW, FH2}; lbr = (intvec3d_t){seg_x2, -SSW, FH2};
            push_triangles(Yp, utl, utr, ltl, ltr); //top
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right
            push_triangles(Yn, ubl, ubr, lbl, lbr); //bottom - facing hollow triangle

            utl.z = FH2; ubl.z = FH2; utr.z = FH2; ubr.z = FH2;
            ltl.z = FH3; lbl.z = FH3; ltr.z = FH3; lbr.z = FH3;
            push_triangles(Yp, utl, utr, ltl, ltr); //top
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right

            utl.z = FH3; ubl.z = FH3; utr.z = FH3; ubr.z = FH3;
            ltl.z = FH4; lbl.z = FH4; ltr.z = FH4; lbr.z = FH4;
            push_triangles(Yp, utl, utr, ltl, ltr); //top
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right

            utl.z = FH4; ubl.z = FH4; utr.z = FH4; ubr.z = FH4;
            ltl.z = FH5; lbl.z = FH5; ltr.z = FH5; lbr.z = FH5;
            push_triangles(Yp, utl, utr, ltl, ltr); //top
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right
            push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face

            TBC += SSW;

            // Y-interval segments
            for (int i=0; i< support_intervalsY; i++) {

                // triangle gap layer
                int32_t FY1 = TBC;
                int32_t FY2 = TBC+HTW;
                int32_t FY3 = TBC+HTW+IG;
                int32_t FY4 = TBC+HTW+IG+HTW;
                int32_t FY5;
                int32_t FY6;

                utl = (intvec3d_t){seg_x1, -FY1, FH1}; utr = (intvec3d_t){seg_x2, -FY1, FH1};
                ubl = (intvec3d_t){seg_x1, -FY2, FH1}; ubr = (intvec3d_t){seg_x2, -FY2, FH1};
                ltl = (intvec3d_t){seg_x1, -FY1, FH2}; ltr = (intvec3d_t){seg_x2, -FY1, FH2};
                lbl = (intvec3d_t){seg_x1, -FY2, FH2}; lbr = (intvec3d_t){seg_x2, -FY2, FH2};
                push_triangles(Xn, utl, ubl, lbl); //left
                push_triangles(Xp, utr, ubr, lbr); //right
                push_triangles(YnZn, utl, utr, lbl, lbr); //diagonal downward face

                utl = (intvec3d_t){seg_x1, -FY2, FH1}; utr = (intvec3d_t){seg_x2, -FY2, FH1};
                ubl = (intvec3d_t){seg_x1, -FY3, FH1}; ubr = (intvec3d_t){seg_x2, -FY3, FH1};
                ltl = (intvec3d_t){seg_x1, -FY2, FH2}; ltr = (intvec3d_t){seg_x2, -FY2, FH2};
                lbl = (intvec3d_t){seg_x1, -FY3, FH2}; lbr = (intvec3d_t){seg_x2, -FY3, FH2};
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right

                utl = (intvec3d_t){seg_x1, -FY3, FH1}; utr = (intvec3d_t){seg_x2, -FY3, FH1};
                ubl = (intvec3d_t){seg_x1, -FY4, FH1}; ubr = (intvec3d_t){seg_x2, -FY4, FH1};
                ltl = (intvec3d_t){seg_x1, -FY3, FH2}; ltr = (intvec3d_t){seg_x2, -FY3, FH2};
                lbl = (intvec3d_t){seg_x1, -FY4, FH2}; lbr = (intvec3d_t){seg_x2, -FY4, FH2};
                push_triangles(Xn, utl, ubl, ltl); //left
                push_triangles(Xp, utr, ubr, ltr); //right
                push_triangles(YpZn, ubl, ubr, ltl, ltr); //diagonal downward face


                // solid foot layer
                FY1 = TBC;
                FY2 = TBC+HSW;
                FY3 = TBC+HSW+TSG;
                FY4 = TBC+HSW+TSG+IG;
                FY5 = TBC+HSW+TSG+IG+TSG;
                FY6 = TBC+HSW+TSG+IG+TSG+HSW;

                utl = (intvec3d_t){seg_x1, -FY1, FH2}; utr = (intvec3d_t){seg_x2, -FY1, FH2};
                ubl = (intvec3d_t){seg_x1, -FY2, FH2}; ubr = (intvec3d_t){seg_x2, -FY2, FH2};
                ltl = (intvec3d_t){seg_x1, -FY1, FH3}; ltr = (intvec3d_t){seg_x2, -FY1, FH3};
                lbl = (intvec3d_t){seg_x1, -FY2, FH3}; lbr = (intvec3d_t){seg_x2, -FY2, FH3};
                push_triangles(Zp, utl, ubl, utr, ubr); //upper face
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right

                utl = (intvec3d_t){seg_x1, -FY2, FH2}; utr = (intvec3d_t){seg_x2, -FY2, FH2};
                ubl = (intvec3d_t){seg_x1, -FY3, FH2}; ubr = (intvec3d_t){seg_x2, -FY3, FH2};
                ltl = (intvec3d_t){seg_x1, -FY2, FH3}; ltr = (intvec3d_t){seg_x2, -FY2, FH3};
                lbl = (intvec3d_t){seg_x1, -FY3, FH3}; lbr = (intvec3d_t){seg_x2, -FY3, FH3};
                push_triangles(Zp, utl, ubl, utr, ubr); //upper face
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right
                push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face

                utl = (intvec3d_t){seg_x1, -FY3, FH2}; utr = (intvec3d_t){seg_x2, -FY3, FH2};
                ubl = (intvec3d_t){seg_x1, -FY4, FH2}; ubr = (intvec3d_t){seg_x2, -FY4, FH2};
                ltl = (intvec3d_t){seg_x1, -FY3, FH3}; ltr = (intvec3d_t){seg_x2, -FY3, FH3};
                lbl = (intvec3d_t){seg_x1, -FY4, FH3}; lbr = (intvec3d_t){seg_x2, -FY4, FH3};
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right
                push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face

                utl = (intvec3d_t){seg_x1, -FY4, FH2}; utr = (intvec3d_t){seg_x2, -FY4, FH2};
                ubl = (intvec3d_t){seg_x1, -FY5, FH2}; ubr = (intvec3d_t){seg_x2, -FY5, FH2};
                ltl = (intvec3d_t){seg_x1, -FY4, FH3}; ltr = (intvec3d_t){seg_x2, -FY4, FH3};
                lbl = (intvec3d_t){seg_x1, -FY5, FH3}; lbr = (intvec3d_t){seg_x2, -FY5, FH3};
                push_triangles(Zp, utl, ubl, utr, ubr); //upper face
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right
                push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face

                utl = (intvec3d_t){seg_x1, -FY5, FH2}; utr = (intvec3d_t){seg_x2, -FY5, FH2};
                ubl = (intvec3d_t){seg_x1, -FY6, FH2}; ubr = (intvec3d_t){seg_x2, -FY6, FH2};
                ltl = (intvec3d_t){seg_x1, -FY5, FH3}; ltr = (intvec3d_t){seg_x2, -FY5, FH3};
                lbl = (intvec3d_t){seg_x1, -FY6, FH3}; lbr = (intvec3d_t){seg_x2, -FY6, FH3};
                push_triangles(Zp, utl, ubl, utr, ubr); //upper face
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right


                // support gap strip
                utl = (intvec3d_t){seg_x1, -FY1, FH3}; utr = (intvec3d_t){seg_x2, -FY1, FH3};
                ubl = (intvec3d_t){seg_x1, -FY2, FH3}; ubr = (intvec3d_t){seg_x2, -FY2, FH3};
                ltl = (intvec3d_t){seg_x1, -FY1, FH4}; ltr = (intvec3d_t){seg_x2, -FY1, FH4};
                lbl = (intvec3d_t){seg_x1, -FY2, FH4}; lbr = (intvec3d_t){seg_x2, -FY2, FH4};
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right
                push_triangles(Yn, ubl, ubr, lbl, lbr); //bottom

                utl = (intvec3d_t){seg_x1, -FY5, FH3}; utr = (intvec3d_t){seg_x2, -FY5, FH3};
                ubl = (intvec3d_t){seg_x1, -FY6, FH3}; ubr = (intvec3d_t){seg_x2, -FY6, FH3};
                ltl = (intvec3d_t){seg_x1, -FY5, FH4}; ltr = (intvec3d_t){seg_x2, -FY5, FH4};
                lbl = (intvec3d_t){seg_x1, -FY6, FH4}; lbr = (intvec3d_t){seg_x2, -FY6, FH4};
                push_triangles(Yp, utl, utr, ltl, ltr); //top
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right


                // solid base layer
                utl = (intvec3d_t){seg_x1, -FY1, FH4}; utr = (intvec3d_t){seg_x2, -FY1, FH4};
                ubl = (intvec3d_t){seg_x1, -FY2, FH4}; ubr = (intvec3d_t){seg_x2, -FY2, FH4};
                ltl = (intvec3d_t){seg_x1, -FY1, FH5}; ltr = (intvec3d_t){seg_x2, -FY1, FH5};
                lbl = (intvec3d_t){seg_x1, -FY2, FH5}; lbr = (intvec3d_t){seg_x2, -FY2, FH5};
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right
                push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face

                utl = (intvec3d_t){seg_x1, -FY2, FH4}; utr = (intvec3d_t){seg_x2, -FY2, FH4};
                ubl = (intvec3d_t){seg_x1, -FY5, FH4}; ubr = (intvec3d_t){seg_x2, -FY5, FH4};
                ltl = (intvec3d_t){seg_x1, -FY2, FH5}; ltr = (intvec3d_t){seg_x2, -FY2, FH5};
                lbl = (intvec3d_t){seg_x1, -FY5, FH5}; lbr = (intvec3d_t){seg_x2, -FY5, FH5};
                push_triangles(Zp, utl, ubl, utr, ubr); //upper face
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right
                push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face

                utl = (intvec3d_t){seg_x1, -FY5, FH4}; utr = (intvec3d_t){seg_x2, -FY5, FH4};
                ubl = (intvec3d_t){seg_x1, -FY6, FH4}; ubr = (intvec3d_t){seg_x2, -FY6, FH4};
                ltl = (intvec3d_t){seg_x1, -FY5, FH5}; ltr = (intvec3d_t){seg_x2, -FY5, FH5};
                lbl = (intvec3d_t){seg_x1, -FY6, FH5}; lbr = (intvec3d_t){seg_x2, -FY6, FH5};
                push_triangles(Xn, utl, ubl, ltl, lbl); //left
                push_triangles(Xp, utr, ubr, ltr, lbr); //right
                push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face


                TBC += (HTW + IG + HTW);
            }

            // bottom side support strip
            utl = (intvec3d_t){seg_x1,  -TBC, FH1}; utr = (intvec3d_t){seg_x2, -TBC, FH1};
            ubl = (intvec3d_t){seg_x1, -BS, FH1}; ubr = (intvec3d_t){seg_x2, -BS, FH1};
            ltl = (intvec3d_t){seg_x1,  -TBC, FH2}; ltr = (intvec3d_t){seg_x2, -TBC, FH2};
            lbl = (intvec3d_t){seg_x1, -BS, FH2}; lbr = (intvec3d_t){seg_x2, -BS, FH2};
            push_triangles(Yp, utl, utr, ltl, ltr); //top - facing hollow triangle
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right
            push_triangles(Yn, ubl, ubr, lbl, lbr); //bottom

            utl.z = FH2; ubl.z = FH2; utr.z = FH2; ubr.z = FH2;
            ltl.z = FH3; lbl.z = FH3; ltr.z = FH3; lbr.z = FH3;
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right
            push_triangles(Yn, ubl, ubr, lbl, lbr); //bottom

            utl.z = FH3; ubl.z = FH3; utr.z = FH3; ubr.z = FH3;
            ltl.z = FH4; lbl.z = FH4; ltr.z = FH4; lbr.z = FH4;
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right
            push_triangles(Yn, ubl, ubr, lbl, lbr); //bottom

            utl.z = FH4; ubl.z = FH4; utr.z = FH4; ubr.z = FH4;
            ltl.z = FH5; lbl.z = FH5; ltr.z = FH5; lbr.z = FH5;
            push_triangles(Xn, utl, ubl, ltl, lbl); //left
            push_triangles(Xp, utr, ubr, ltr, lbr); //right
            push_triangles(Yn, ubl, ubr, lbl, lbr); //bottom
            push_triangles(Zn, ltl, lbl, ltr, lbr); //lower face
        }
    }

    if (foot.mode == pyramids) {

        // PREPARE PYRAMID SUPPORTS + FOOT(?) + LOWER STRIP(?)
            // DONE ABOVE: float body_size = (BS * raster_size.as_mm()) / UVstretchXY;
            int32_t SW = w;
            float set_width = (SW * raster_size.as_mm()) / UVstretchXY; // TODO: check stretch divide?

            int32_t pyramid_count_Y = int(round(body_size/(foot.pyramid_pitch.as_mm()))); // TODO: StretchXY?
            float final_pyramid_pitch_Y = body_size / pyramid_count_Y; // TODO: StretchXY?

            int32_t pyramid_count_X = int(round(set_width/(foot.pyramid_pitch.as_mm()))); // TODO: StretchXY?
            float final_pyramid_pitch_X = set_width / pyramid_count_X; // TODO: StretchXY?

            int32_t PH = int(round(
                            (final_pyramid_pitch_Y-foot.pyramid_top_length.as_mm())
                            * 0.5 * foot.pyramid_height_factor * UVstretchZ / layer_height.as_mm()));

            int32_t PTCH = int(round(
                            foot.pyramid_top_column_height.as_mm() * UVstretchZ / layer_height.as_mm()
                            ));

            logger.INFO() << "pyramid_count_Y: " << pyramid_count_Y << std::endl;
            logger.INFO() << "final_pyramid_pitch_Y: " << final_pyramid_pitch_Y << std::endl;

            logger.INFO() << "pyramid_count_X: " << pyramid_count_X << std::endl;
            logger.INFO() << "final_pyramid_pitch_X: " << final_pyramid_pitch_X << std::endl;

        std::vector<int32_t> pyramid_base_Y_points;
        std::vector<int32_t> pyramid_top_Y_points;

        std::vector<int32_t> pyramid_base_X_points;
        std::vector<int32_t> pyramid_top_X_points;

        for (i=0; i<pyramid_count_Y; i++) {
            int32_t stepY = int(round(i*final_pyramid_pitch_Y*UVstretchXY/raster_size.as_mm()));
            pyramid_base_Y_points.push_back(stepY);
        }
        pyramid_base_Y_points.push_back(BS);

        for (i=0; i<pyramid_count_X; i++) {
            int32_t stepX = int(round(i*final_pyramid_pitch_X*UVstretchXY/raster_size.as_mm()));
            pyramid_base_X_points.push_back(stepX);
        }
        pyramid_base_X_points.push_back(SW);

        for (i=0; i<pyramid_count_Y; i++) {
            float firstY = (((float)i+0.5)*final_pyramid_pitch_Y)-(foot.pyramid_top_length.as_mm()/2);
            pyramid_top_Y_points.push_back(int(round((firstY)*UVstretchXY/raster_size.as_mm())));
            float secondY = (((float)i+0.5)*final_pyramid_pitch_Y)+(foot.pyramid_top_length.as_mm()/2);
            pyramid_top_Y_points.push_back(int(round((secondY)*UVstretchXY/raster_size.as_mm())));
        }

        for (i=0; i<pyramid_count_X; i++) {
            float firstX = (((float)i+0.5)*final_pyramid_pitch_X)-(foot.pyramid_top_length.as_mm()/2);
            pyramid_top_X_points.push_back(int(round((firstX)*UVstretchXY/raster_size.as_mm())));
            float secondX = (((float)i+0.5)*final_pyramid_pitch_X)+(foot.pyramid_top_length.as_mm()/2);
            pyramid_top_X_points.push_back(int(round((secondX)*UVstretchXY/raster_size.as_mm())));
        }

        intvec3d_t XpZp = (intvec3d_t){ 1,  0,  1};  intvec3d_t XnZp = (intvec3d_t){-1,  0,  1};
        intvec3d_t YpZp = (intvec3d_t){ 0,  1,  1};  intvec3d_t YnZp = (intvec3d_t){0, -1,  1};

        // LOWER BODY STRIP
        intvec3d_t vc, v1, v2;

        // left face
        vc = (intvec3d_t){0, -h/2, -(BLC+BH)/2};
        for (i=0; i<pyramid_count_Y; i++) {
            intvec3d_t v1 = (intvec3d_t){0, -pyramid_base_Y_points[i], -BH};
            intvec3d_t v2 = (intvec3d_t){0, -pyramid_base_Y_points[i+1], -BH};
            push_triangles(Xn, v1, v2, vc);
        }
        v1 = (intvec3d_t){0, 0, -BH};
        v2 = (intvec3d_t){0, 0, -BLC};
        push_triangles(Xn, v1, v2, vc);
        v1 = (intvec3d_t){0, -h, -BLC};
        push_triangles(Xn, v1, v2, vc);
        v2 = (intvec3d_t){0, -h, -BH};
        push_triangles(Xn, v1, v2, vc);

        // right face
        vc = (intvec3d_t){w, -h/2, -(BLC+BH)/2};
        for (i=0; i<pyramid_count_Y; i++) {
            intvec3d_t v1 = (intvec3d_t){w, -pyramid_base_Y_points[i], -BH};
            intvec3d_t v2 = (intvec3d_t){w, -pyramid_base_Y_points[i+1], -BH};
            push_triangles(Xp, v1, v2, vc);
        }
        v1 = (intvec3d_t){w, 0, -BH};
        v2 = (intvec3d_t){w, 0, -BLC};
        push_triangles(Xp, v1, v2, vc);
        v1 = (intvec3d_t){w, -h, -BLC};
        push_triangles(Xp, v1, v2, vc);
        v2 = (intvec3d_t){w, -h, -BH};
        push_triangles(Xp, v1, v2, vc);

        //top face
        vc = (intvec3d_t){w/2, 0, -(BLC+BH)/2};
        for (i=0; i<pyramid_count_X; i++) {
            intvec3d_t v1 = (intvec3d_t){pyramid_base_X_points[i], 0, -BH};
            intvec3d_t v2 = (intvec3d_t){pyramid_base_X_points[i+1], 0, -BH};
            push_triangles(Yp, v1, v2, vc);
        }
        v1 = (intvec3d_t){0, 0, -BH};
        v2 = (intvec3d_t){0, 0, -BLC};
        push_triangles(Yp, v1, v2, vc);
        v1 = (intvec3d_t){w, 0, -BLC};
        push_triangles(Yp, v1, v2, vc);
        v2 = (intvec3d_t){w, 0, -BH};
        push_triangles(Yp, v1, v2, vc);

        //bottom face
        vc = (intvec3d_t){w/2, -h, -(BLC+BH)/2};
        for (i=0; i<pyramid_count_X; i++) {
            intvec3d_t v1 = (intvec3d_t){pyramid_base_X_points[i], -h, -BH};
            intvec3d_t v2 = (intvec3d_t){pyramid_base_X_points[i+1], -h, -BH};
            push_triangles(Yn, v1, v2, vc);
        }
        v1 = (intvec3d_t){0, -h, -BH};
        v2 = (intvec3d_t){0, -h, -BLC};
        push_triangles(Yn, v1, v2, vc);
        v1 = (intvec3d_t){w, -h, -BLC};
        push_triangles(Yn, v1, v2, vc);
        v2 = (intvec3d_t){w, -h, -BH};
        push_triangles(Yn, v1, v2, vc);

        for (i=0; i<pyramid_count_Y; i++) {
            for (j=0; j<pyramid_count_X; j++) {

                utl = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2], -BH};
                utr = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2+1], -BH};
                ubl = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2], -BH};
                ubr = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2+1], -BH};

                // type bottom layer, downward-facing
                ltl = (intvec3d_t){pyramid_base_X_points[j], -pyramid_base_Y_points[i], -(BH)};
                ltr = (intvec3d_t){pyramid_base_X_points[j], -pyramid_base_Y_points[i+1], -(BH)};
                lbl = (intvec3d_t){pyramid_base_X_points[j+1], -pyramid_base_Y_points[i], -(BH)};
                lbr = (intvec3d_t){pyramid_base_X_points[j+1], -pyramid_base_Y_points[i+1], -(BH)};

                push_triangles(Zn, ubl, utl, ltl, lbl); // left face
                push_triangles(Zn, ubr, ltr, utr, lbr); // right face
                push_triangles(Zn, utl, utr, ltr, ltl); // top face
                push_triangles(Zn, ubl, lbr, ubr, lbl); // bottom face



                ////////// TODO Add columns on top of to pyramids
                utl = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2], -BH};
                utr = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2+1], -BH};
                ubl = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2], -BH};
                ubr = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2+1], -BH};

                ltl = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2], -(BH+PTCH)};
                ltr = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2+1], -(BH+PTCH)};
                lbl = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2], -(BH+PTCH)};
                lbr = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2+1], -(BH+PTCH)};

                push_triangles(Xn, ubl, utl, ltl, lbl); // left face
                push_triangles(Xp, ubr, ltr, utr, lbr); // right face
                push_triangles(Yn, utl, utr, ltr, ltl); // top face
                push_triangles(Yp, ubl, lbr, ubr, lbl); // bottom face                /////////////////

                // pyramids
                utl = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2], -(BH+PTCH)};
                utr = (intvec3d_t){pyramid_top_X_points[j*2], -pyramid_top_Y_points[i*2+1], -(BH+PTCH)};
                ubl = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2], -(BH+PTCH)};
                ubr = (intvec3d_t){pyramid_top_X_points[j*2+1], -pyramid_top_Y_points[i*2+1], -(BH+PTCH)};

                ltl = (intvec3d_t){pyramid_base_X_points[j], -pyramid_base_Y_points[i], -(BH+PTCH+PH)};
                ltr = (intvec3d_t){pyramid_base_X_points[j], -pyramid_base_Y_points[i+1], -(BH+PTCH+PH)};
                lbl = (intvec3d_t){pyramid_base_X_points[j+1], -pyramid_base_Y_points[i], -(BH+PTCH+PH)};
                lbr = (intvec3d_t){pyramid_base_X_points[j+1], -pyramid_base_Y_points[i+1], -(BH+PTCH+PH)};

                // TODO: Maybe adjust normal vector based on pyramid angle?
                push_triangles(XnZp, ubl, utl, ltl, lbl); // left face
                push_triangles(XpZp, ubr, ltr, utr, lbr); // right face
                push_triangles(YnZp, utl, utr, ltr, ltl); // top face
                push_triangles(YpZp, ubl, lbr, ubr, lbl); // bottom face


            }
        }

        // FOOT STRIP

        // left face
        vc = (intvec3d_t){0, -h/2, -BH-(PFH+PTCH+PH)/2};
        for (i=0; i<pyramid_count_Y; i++) {
            intvec3d_t v1 = (intvec3d_t){0, -pyramid_base_Y_points[i], -(BH+PTCH+PH)};
            intvec3d_t v2 = (intvec3d_t){0, -pyramid_base_Y_points[i+1], -(BH+PTCH+PH)};
            push_triangles(Xn, v1, v2, vc);
        }
        v1 = (intvec3d_t){0, 0, -(BH+PTCH+PH)};
        v2 = (intvec3d_t){0, 0, -(BH+PFH)};
        push_triangles(Xn, v1, v2, vc);
        v1 = (intvec3d_t){0, -h, -(BH+PFH)};
        push_triangles(Xn, v1, v2, vc);
        v2 = (intvec3d_t){0, -h, -(BH+PTCH+PH)};
        push_triangles(Xn, v1, v2, vc);

        // right face
        vc = (intvec3d_t){w, -h/2, -BH-(PFH+PTCH+PH)/2};
        for (i=0; i<pyramid_count_Y; i++) {
            intvec3d_t v1 = (intvec3d_t){w, -pyramid_base_Y_points[i], -(BH+PTCH+PH)};
            intvec3d_t v2 = (intvec3d_t){w, -pyramid_base_Y_points[i+1], -(BH+PTCH+PH)};
            push_triangles(Xp, v1, v2, vc);
        }
        v1 = (intvec3d_t){w, 0, -(BH+PTCH+PH)};
        v2 = (intvec3d_t){w, 0, -(BH+PFH)};
        push_triangles(Xp, v1, v2, vc);
        v1 = (intvec3d_t){w, -h, -(BH+PFH)};
        push_triangles(Xp, v1, v2, vc);
        v2 = (intvec3d_t){w, -h, -(BH+PTCH+PH)};
        push_triangles(Xp, v1, v2, vc);

        //top face
        vc = (intvec3d_t){w/2, 0, -BH-(PFH+PTCH+PH)/2};
        for (i=0; i<pyramid_count_X; i++) {
            intvec3d_t v1 = (intvec3d_t){pyramid_base_X_points[i], 0, -(BH+PTCH+PH)};
            intvec3d_t v2 = (intvec3d_t){pyramid_base_X_points[i+1], 0, -(BH+PTCH+PH)};
            push_triangles(Yp, v1, v2, vc);
        }
        v1 = (intvec3d_t){0, 0, -(BH+PTCH+PH)};
        v2 = (intvec3d_t){0, 0, -(BH+PFH)};
        push_triangles(Yp, v1, v2, vc);
        v1 = (intvec3d_t){w, 0, -(BH+PFH)};
        push_triangles(Yp, v1, v2, vc);
        v2 = (intvec3d_t){w, 0, -(BH+PTCH+PH)};
        push_triangles(Yp, v1, v2, vc);

        //bottom face
        vc = (intvec3d_t){w/2, -h, -BH-(PFH+PTCH+PH)/2};
        for (i=0; i<pyramid_count_X; i++) {
            intvec3d_t v1 = (intvec3d_t){pyramid_base_X_points[i], -h, -(BH+PTCH+PH)};
            intvec3d_t v2 = (intvec3d_t){pyramid_base_X_points[i+1], -h, -(BH+PTCH+PH)};
            push_triangles(Yn, v1, v2, vc);
        }
        v1 = (intvec3d_t){0, -h, -(BH+PTCH+PH)};
        v2 = (intvec3d_t){0, -h, -(BH+PFH)};
        push_triangles(Yn, v1, v2, vc);
        v1 = (intvec3d_t){w, -h, -(BH+PFH)};
        push_triangles(Yn, v1, v2, vc);
        v2 = (intvec3d_t){w, -h, -(BH+PTCH+PH)};
        push_triangles(Yn, v1, v2, vc);
    }


    // REDUCED FOOT (if exists)

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
        push_triangles(Zn, ubl, utl, ltl, lbl);

        // right downward face
        push_triangles(Zn, ubr, ltr, utr, lbr);

        // top downward face
        push_triangles(Zn, utl, utr, ltr, ltl);

        // bottom downward face
        push_triangles(Zn, ubl, lbr, ubr, lbl);
    }


    if (foot.mode == step) { // stepped foot
        utl = (intvec3d_t){FXY,         -FXY, -(BH-FZ)};
        utr = (intvec3d_t){w - FXY,     -FXY, -(BH-FZ)};
        ubl = (intvec3d_t){FXY,     -h + FXY, -(BH-FZ)};
        ubr = (intvec3d_t){w - FXY, -h + FXY, -(BH-FZ)};

        ltl = (intvec3d_t){FXY,         -FXY, -BH};
        ltr = (intvec3d_t){w - FXY,     -FXY, -BH};
        lbl = (intvec3d_t){FXY,     -h + FXY, -BH};
        lbr = (intvec3d_t){w - FXY, -h + FXY, -BH};

        // left face
        push_triangles(Xn, ubl, utl, ltl, lbl);

        // right face
        push_triangles(Xp, ubr, ltr, utr, lbr);

        // top face
        push_triangles(Yp, utl, utr, ltr, ltl);

        // bottom face
        push_triangles(Yn, ubl, lbr, ubr, lbl);

    }

    if (foot.mode == bevel) { // beveled foot
        utl = (intvec3d_t){0,  0, -(BH-FZ)};
        utr = (intvec3d_t){w,  0, -(BH-FZ)};
        ubl = (intvec3d_t){0, -h, -(BH-FZ)};
        ubr = (intvec3d_t){w, -h, -(BH-FZ)};

        ltl = (intvec3d_t){FXY,         -FXY, -BH};
        ltr = (intvec3d_t){w - FXY,     -FXY, -BH};
        lbl = (intvec3d_t){FXY,     -h + FXY, -BH};
        lbr = (intvec3d_t){w - FXY, -h + FXY, -BH};

        intvec3d_t N;
        N.z = -FXY;

        // left face
        N.x = -FZ; N.y = 0 ;
        push_triangles(N, ubl, utl, ltl, lbl);

        // right face
        N.x = FZ; N.y = 0 ;
        push_triangles(N, ubr, ltr, utr, lbr);

        // top face
        N.y = FZ; N.x = 0 ;
        push_triangles(N, utl, utr, ltr, ltl);

        // bottom face
        N.y = -FZ; N.y = 0 ;
        push_triangles(N, ubl, lbr, ubr, lbl);
    }


    // LOWER SURFACE

    // bevel/step foot
    if ((foot.mode == step) || (foot.mode == bevel)) {
        ltl = (intvec3d_t){FXY,         -FXY, -BH};
        ltr = (intvec3d_t){w - FXY,     -FXY, -BH};
        lbl = (intvec3d_t){FXY,     -h + FXY, -BH};
        lbr = (intvec3d_t){w - FXY, -h + FXY, -BH};
        push_triangles(Zn, ltr, lbl, ltl, lbr);    
    }
    else if (foot.mode == supports) {
        // Done in supports section above?
    }
    else if (foot.mode == pyramids) {
        // lower by pyramid foot height
        ltl = (intvec3d_t){0,  0, -(BH+PFH)};
        ltr = (intvec3d_t){w,  0, -(BH+PFH)};
        lbl = (intvec3d_t){0, -h, -(BH+PFH)};
        lbr = (intvec3d_t){w, -h, -(BH+PFH)};
        push_triangles(Zn, ltr, lbl, ltl, lbr);
    }
    else { // no reduced foot
        ltl = (intvec3d_t){0,  0, -BH};
        ltr = (intvec3d_t){w,  0, -BH};
        lbl = (intvec3d_t){0, -h, -BH};
        lbr = (intvec3d_t){w, -h, -BH};
        push_triangles(Zn, ltr, lbl, ltl, lbr);    
    }

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
        logger.ERROR() << "No OBJ file specified." << std::endl;
        return -1;
    }

    std::ofstream obj_out(filename, std::ios::binary);
    if (!obj_out.is_open()) {
        logger.ERROR() << "Could not open OBJ file " << filename <<" for writing." << std::endl;
        return -1;
    }

    logger.INFO() << "Triangle count is " << triangles.size() << std::endl;

    obj_out << "### OBJ data exported from t3t_pbm2stl:" << std::endl;

    obj_out << std::endl << "# Vertices with coordinates in mm:" << std::endl;
    for (i=1; i<vertices.size(); i++) {
        intvec3d_t vertex = vertices[i];
        float x = vertex.x * RS;
        float y = vertex.y * RS;
        float z = vertex.z * LH;

        obj_out << "v "
                << x << " "
                << y << " "
                << z << " "
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

    logger.INFO() << "Wrote binary STL data to " << filename << std::endl;
    logger.INFO() << "---------------------" << std::endl;
    logger.INFO() << "Exported STL metrics:" << std::endl;
    logger.INFO() << "Type height   " << boost::format("%6.4f") % type_height.as_inch()
              << " inch  |  "  << boost::format("%6.3f") %  type_height.as_mm() << " mm" << std::endl;
    logger.INFO() << "Body size     " << boost::format("%6.4f") % (h*raster_size.as_inch())
              << " inch  |  "  << boost::format("%6.3f") %  (h*raster_size.as_mm()) << " mm" << std::endl;
    logger.INFO() << "Set width     " << boost::format("%6.4f") % (w*raster_size.as_inch())
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
        logger.ERROR() << "No STL file specified." << std::endl;
        return -1;
    }

    std::ofstream stl_out(filename, std::ios::binary);
    if (!stl_out.is_open()) {
        logger.ERROR() << "Could not open STL file " << filename <<" for writing." << std::endl;
        return -1;
    }

    logger.INFO() << "Triangle count is " << triangles.size() << std::endl;

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


    logger.INFO() << "Wrote binary STL data to " << filename << std::endl;
    logger.INFO() << "---------------------" << std::endl;
    logger.INFO() << "Exported STL metrics:" << std::endl;
    logger.INFO() << "Type height   " << boost::format("%6.4f") % type_height.as_inch()
              << " inch  |  "  << boost::format("%6.3f") %  type_height.as_mm() << " mm" << std::endl;
    logger.INFO() << "Body size     " << boost::format("%6.4f") % (h*raster_size.as_inch())
              << " inch  |  "  << boost::format("%6.3f") %  (h*raster_size.as_mm()) << " mm" << std::endl;
    logger.INFO() << "Set width     " << boost::format("%6.4f") % (w*raster_size.as_inch())
              << " inch  |  "  << boost::format("%6.3f") %  (w*raster_size.as_mm()) << " mm" << std::endl;

    return 0;
}

