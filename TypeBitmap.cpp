#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <string>

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