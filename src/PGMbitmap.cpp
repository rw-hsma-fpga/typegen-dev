#include "PGMbitmap.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <boost/format.hpp> 


PGMbitmap::PGMbitmap()
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL) {}


PGMbitmap::PGMbitmap(std::string filename)
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL)
{
    unload();
    if (filename.substr(filename.size()-3,3)=="pbm") {
        loadPBM(filename);
    }
    else
        loadPGM(filename);
}


PGMbitmap::PGMbitmap(uint32_t width, uint32_t height)
            : loaded(false), bm_width(0), bm_height(0), bitmap(NULL)
{
    newBitmap(width, height);
}


int PGMbitmap::newBitmap(uint32_t width, uint32_t height)
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


PGMbitmap::~PGMbitmap() {
    unload();
}


int PGMbitmap::loadPBM(std::string filename)
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
        std::cerr << "ERROR: Not a valid PBM file!" << std::endl;
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
                        std::cerr << "ERROR: More data than specified." << std::endl;
                        free(bitmap);
                        bitmap = NULL;
                        pbm.close();
                        return -1;
                    }

                    if (c=='1')
                        *(runbuf++) = 0;
                    else
                        *(runbuf++) = 255;

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
                std::cerr << "ERROR: More data than specified." << std::endl;
                free(bitmap);
                bitmap = NULL;
                pbm.close();
                return -1;
            }

            for (int i=0; i<8; i++) {

                if (byte & 0x80) { // start on MSB
                    *(runbuf++) = 0;
                }
                else {
                    *(runbuf++) = 255;
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
        std::cerr << "Less data than specified." << std::endl;
        free(bitmap);
        bitmap = NULL;
        pbm.close();
        return -1;
    }

    max_value = 255;
    loaded = true;
    pbm.close();
    return 0;
}

// TODO actually make PGM reader
int PGMbitmap::loadPGM(std::string filename)
{
    std::string linebuf;
    uint32_t size;

    std::ifstream pgm(filename);
    if (!pgm.is_open())
        return -1;

    getline(pgm, linebuf);
    std::erase(linebuf, '\r');

    uint8_t format;
    if (linebuf=="P1") {
        format = 1;
    }
    else if (linebuf=="P4") {
        format = 4;
    }
    else {
        std::cerr << "ERROR: Not a valid PBM file!" << std::endl;
        pgm.close();
        return -1;
    }

    // remove comments
    while(pgm.peek()=='#')
        getline(pgm, linebuf);

    if (!(pgm >> bm_width) || (bm_width==0)) {
        std::cerr << "ERROR: No valid X dimension" << std::endl;
        pgm.close();
        return -1;
    }

    // remove comments
    while(pgm.peek()=='#')
        getline(pgm, linebuf);

    if (!(pgm >> bm_height) || (bm_height==0)) {
        std::cerr << "ERROR: No valid Y dimension" << std::endl;
        pgm.close();
        return -1;
    }

    bitmap = (uint8_t*)calloc(size = bm_width*bm_height, sizeof(uint8_t));
    if (bitmap == NULL) {
        std::cerr << "ERROR: Bitmap buffer allocation failed" << std::endl;
        pgm.close();
        return -1;
    }

    uint32_t count = 0;
    uint8_t *runbuf = bitmap;

    // ASCII format (P1)
    if (format==1) {
        char c;

        c = pgm.get();
        while(!pgm.eof()) {
            switch(c) {
                case '#':
                    getline(pgm, linebuf); break;
                case '0':
                case '1':
                    if (count == size) {
                        std::cerr << "ERROR: More data than specified." << std::endl;
                        free(bitmap);
                        bitmap = NULL;
                        pgm.close();
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
            c = pgm.get();
        }
    }

    // Binary format (P4)
    if (format==4) {
        uint8_t byte;
        uint32_t x_cnt = 0;
        uint32_t y_cnt = 0;

        byte = (uint8_t)pgm.get(); // whitespace after height
        
        byte = (uint8_t)pgm.get();
        while(!pgm.eof()) {
            if (count >= size) { // file should have ended
                std::cerr << "ERROR: More data than specified." << std::endl;
                free(bitmap);
                bitmap = NULL;
                pgm.close();
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
            byte = (uint8_t)pgm.get();
        }
    }

    if (count != size) {
        std::cerr << "Less data than specified." << std::endl;
        free(bitmap);
        bitmap = NULL;
        pgm.close();
        return -1;
    }

    loaded = true;
    pgm.close();
    return 0;
}



void PGMbitmap::unload()
{
    if (bitmap != NULL) {
        free(bitmap);
    }
    bitmap = NULL;
    loaded = false;
}


bool PGMbitmap::is_loaded()
{
    return loaded;
}


int PGMbitmap::storePGM(std::string filename)
{
    uint8_t *bm_ptr;
    uint32_t x, y;

    if (!loaded) {
        std::cerr << "ERROR: No bitmap to store." << std::endl;
        return -1;
    }

    std::ofstream pgm(filename);
    if (!pgm.is_open()) {
        std::cerr << "ERROR: Opening " << filename <<" for writing failed." << std::endl;
        return -1;
    }

    bm_ptr = bitmap;

    pgm << "P2" << std::endl;
    pgm << bm_width << " " << bm_height << std::endl;
    pgm << (uint32_t)max_value << std::endl;

    for (y=0; y<bm_height ; y++) {
        for (x=0; x<bm_width; x++) {
            pgm << (uint32_t)*bm_ptr++ <<" ";
        }
        pgm << std::endl;
    }

    pgm.close();
    return 0;
}


int PGMbitmap::pasteGlyph(uint8_t *glyph, uint32_t g_width, uint32_t g_height, uint32_t top_pos, uint32_t left_pos)
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


void PGMbitmap::threshold(uint8_t thr) {

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


void PGMbitmap::mirror() {

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
