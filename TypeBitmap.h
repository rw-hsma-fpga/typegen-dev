#ifndef TYPEBITMAP_H
#define TYPEBITMAP_H

#include <cstdint>
#include <string>
#include <vector>
#include "t3t_support_types.h"

enum reduced_foot_mode {none, bevel, step};

class TypeBitmap {
    bool loaded;
    uint8_t *bitmap;
    uint32_t bm_width;
    uint32_t bm_height;

    // for optimized STL conversion
    int32_t *tag_bitmap_i32;

    struct STLrect {
        int32_t top, left, bottom, right;
        int32_t tag;
    };
    std::vector<STLrect> glyph_rects;
    std::vector<STLrect> body_rects;

    // for optimized OBJ conversion
    struct OBJtriangle {
        uint32_t v1, v2, v3; // vertex indices
    };
    std::vector<intvec3d_t> vertices;
    std::vector<OBJtriangle> triangles;

    dim_t type_height;
    dim_t depth_of_drive;
    dim_t raster_size;
    dim_t layer_height; // not really required here
    // TODO: Fudge factor Z?

    inline void STL_triangle_write(std::ofstream &outfile, pos3d_t N,
                                   pos3d_t v1, pos3d_t v2, pos3d_t v3,
                                   uint32_t &count);

    void STL_rect_write(std::ofstream &outfile, pos3d_t N,
                                   pos3d_t v1, pos3d_t v2, pos3d_t v3, pos3d_t v4,
                                   uint32_t &count);

    void OBJ_rect_push(intvec3d_t N,
                       intvec3d_t v1, intvec3d_t v2,
                       intvec3d_t v3, intvec3d_t v4);

    uint32_t find_or_add_vertex(intvec3d_t v);


    public:
        TypeBitmap();
        TypeBitmap(std::string filename);
        TypeBitmap(uint32_t width, uint32_t height);
        ~TypeBitmap();

        void unload();
        int load(std::string filename);
        bool is_loaded();

        int store(std::string filename);
        int newBitmap(uint32_t width, uint32_t height);
        int pasteGlyph(uint8_t *glyph, uint32_t g_width, uint32_t g_height, uint32_t top_pos, uint32_t left_pos);
        void threshold(uint8_t thr);
        void mirror();

        int set_type_parameters(dim_t TH, dim_t DOD, dim_t RS, dim_t LH);

        int export_STL(std::string filename, reduced_foot_mode foot_mode,
                       dim_t footXY, dim_t footZ, float UVstretchZ);
        int export_OBJ(std::string filename, reduced_foot_mode foot_mode,
                       dim_t footXY, dim_t footZ, float UVstretchZ);

};

#endif // TYPEBITMAP_H