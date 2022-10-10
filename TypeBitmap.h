#ifndef TYPEBITMAP_H
#define TYPEBITMAP_H

#include <cstdint>
#include <string>


class TypeBitmap {
    bool loaded;
    uint32_t width;
    uint32_t height;
    uint8_t *bitmap;

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