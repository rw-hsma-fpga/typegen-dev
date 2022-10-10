#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>


int main()
{

    std::string bitmap_file = "E0.pbm";

    TypeBitmap *BM = new TypeBitmap(bitmap_file);

    //std::ifstream;


/*
    YAML::Node newconf;

    YAML::Node submap;
    submap["First name"] = "Klaus";
    submap["Last name"] = "Mueller";
    submap["color"] = "violet";

    newconf["Person"].push_back(submap);
*/
    return 0;
}