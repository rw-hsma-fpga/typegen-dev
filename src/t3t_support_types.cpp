#include "t3t_support_types.h"



dim_t::dim_t()
{
    set(0, "mm");
}

dim_t::dim_t(float v, dim_unit_t u)
{
    set(v, u);
}

dim_t::dim_t(float v, std::string ustr)
{
    set(v, ustr);
}

int dim_t::set(float v, dim_unit_t u)
{
    value = v;
    unit = mm;
    if ( (u==mm) || (u==inch) || (u==pt) ) {
        unit = u;
        return 0;
    }
    // No proper unit
    value = 0;
    return -1;
}

int dim_t::set(float v, std::string ustr)
{
    if (ustr=="inch")
        return set(v, inch);

    if (ustr=="mm")
        return set(v, mm);

    if (ustr=="pt") 
        return set(v, pt);

    // No proper unit
    value = 0;
    return -1;
}

float dim_t::as_mm()
{
    if (value==0)
        return 0;

    switch(unit) {
        case mm:
            return value;
        case inch:
            return (value * MM_PER_INCH);
        case pt:
            return (value * INCH_PER_PT * MM_PER_INCH);
        default:
            return 0;
    }
}

float dim_t::as_inch()
{
    if (value==0)
        return 0;

    switch(unit) {
        case mm:
            return (value / MM_PER_INCH);
        case inch:
            return value;
        case pt:
            return (value * INCH_PER_PT);
        default:
            return 0;
    }
}

float dim_t::as_pt()
{
    if (value==0)
        return 0;

    switch(unit) {
        case mm:
            return ((value / INCH_PER_PT) / MM_PER_INCH);
        case inch:
            return (value / INCH_PER_PT);
        case pt:
            return value;
        default:
            return 0;
   }
}