#ifndef T3T_SUPPORT_TYPES_H
#define T3T_SUPPORT_TYPES_H

#include <cstdint>
#include <string>


const float MM_PER_INCH = 25.4;
const float INCH_PER_PT = 0.013835; // per type history; 72pt=0.99612"
// const float INCH_PER_PT = 0.013888; // as 1/72th of an inch


enum dim_unit_t { mm, inch, pt };

class dim_t {
    float value; // 0 equates to invalid,
                 // assuming it's not a useful dimension;
                 // 0 is also the same in any unit
    dim_unit_t unit;

    public:
    dim_t();
    dim_t(float v, dim_unit_t u);
    dim_t(float v, std::string ustr);
    //dim_t& dim_t::operator=(const dim_t&);
    int set(float v, dim_unit_t u);
    int set(float v, std::string ustr);
    float as_mm();
    float as_inch();
    float as_pt();
};


struct pos3d_t {
    float x, y, z;
};

struct intvec2d_t {
    int32_t x, y;
};

struct intvec3d_t {
    int32_t x, y, z;
};



//struct stl_binary_triangle
struct stl_tri_t {
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
    uint16_t attr_cnt;
};

#endif // T3T_SUPPORT_TYPES_H
