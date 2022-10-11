#ifndef TYPEBITMAP_H
#define TYPEBITMAP_H

#include <cstdint>
#include <string>
#include "t3t_support_types.h"


class TypeBitmap {
    bool loaded;
    uint8_t *bitmap;
    uint32_t bm_width;
    uint32_t bm_height;

    inline void STL_triangle_write(std::ofstream &outfile, pos3d_t N,
                                   pos3d_t v1, pos3d_t v2, pos3d_t v3,
                                   uint32_t &count);

    public:
        TypeBitmap();
        TypeBitmap(std::string filename);
        ~TypeBitmap();
        int load(std::string filename);
        void unload();
        bool is_loaded();
        int export_STL(std::string filename);
};

#endif // TYPEBITMAP_H