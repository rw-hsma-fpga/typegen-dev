#ifndef TYPEBITMAP_H
#define TYPEBITMAP_H

#include <cstdint>
#include <string>
#include <vector>
#include "t3t_support_types.h"

enum reduced_foot_mode { no_foot , bevel, step};

struct reduced_foot {
    reduced_foot_mode mode;
    dim_t XY; // reduction on each edge
    dim_t Z;  // height of reduction
    reduced_foot() { mode = no_foot; };
};

enum nick_type{ nick_undefined, flat, triangle, rect, circle};

struct nick {
    nick_type type;
    dim_t z;
    dim_t y;
    nick() { type = nick_undefined; }
};

class TypeBitmap {
    bool loaded;
    uint8_t *bitmap;
    uint32_t bm_width;
    uint32_t bm_height;

    // for optimized mesh conversion (enlarged rects)
    int32_t *tag_bitmap_i32;

    struct STLrect {
        int32_t top, left, bottom, right; // u32?
        int32_t width, height; // u32?
        int32_t tag;
    };
    std::vector<STLrect> glyph_rects;
    std::vector<STLrect> body_rects;

    // for optimized mesh conversion
    struct mesh_triangle {
        intvec3d_t N;
        uint32_t v1, v2, v3; // vertex indices
    };
    std::vector<intvec3d_t> vertices;
    std::vector<mesh_triangle> triangles;

    dim_t type_height;
    dim_t depth_of_drive;
    dim_t raster_size;
    dim_t layer_height; // not really required here

    void push_triangles(intvec3d_t N,
                       intvec3d_t v1, intvec3d_t v2, intvec3d_t v3,
                       intvec3d_t v4 = {0, 0, INT32_MAX});

    void fill_rectangle(int32_t *bm32, STLrect rect); // not needed
    int find_rectangles(void);

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

        int generateMesh(reduced_foot foot, std::vector<nick> &nicks, float UVstretchXY, float UVstretchZ);
        int writeOBJ(std::string filename);
        int writeSTL(std::string filename);
};

#endif // TYPEBITMAP_H