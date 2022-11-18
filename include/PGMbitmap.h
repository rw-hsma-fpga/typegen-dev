#ifndef PGMBITMAP_H
#define PGMBITMAP_H

#include <cstdint>
#include <string>
#include <vector>
#include "t3t_support_types.h"

class PGMbitmap {
    bool loaded;
    uint8_t *bitmap;
    uint32_t bm_width;
    uint32_t bm_height;
    uint8_t max_value;

    public:
        PGMbitmap();
        PGMbitmap(std::string filename);
        PGMbitmap(uint32_t width, uint32_t height);
        PGMbitmap(uint32_t width, uint32_t height, uint8_t color);        
        ~PGMbitmap();

        void unload();
        uint32_t getWidth();
        uint32_t getHeight();
        uint8_t* getAddress();
        static int parsePBM(std::string filename, uint32_t &width, uint32_t &height);
        int loadPBM(std::string filename);
        int loadPGM(std::string filename);
        bool is_loaded();

        int storePGM(std::string filename);
        int newBitmap(uint32_t width, uint32_t height);
        int pasteGlyph(uint8_t *glyph, uint32_t g_width, uint32_t g_height, uint32_t top_pos, uint32_t left_pos);
        int pastePGM(PGMbitmap &PGM, int32_t top_pos, int32_t left_pos, uint8_t bg_shade = 255);
        void threshold(uint8_t thr);
        void mirror();
        void fill(uint8_t);
};

#endif // PGMBITMAP_H