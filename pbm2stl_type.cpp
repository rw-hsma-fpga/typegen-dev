#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>


int main()
{

    std::string bitmap_file = "i.pbm";
    std::string stl_file = "i_new.stl";

    TypeBitmap *BM = new TypeBitmap(bitmap_file);
    BM->export_STL(stl_file);

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