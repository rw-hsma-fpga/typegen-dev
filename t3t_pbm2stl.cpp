#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
using namespace std;

int main()
{

    dim_t testdim;
    cout << "-- unset dim_t --" << endl;
    cout << testdim.as_mm() << " mm" << endl;
    cout << testdim.as_inch() << " inch" << endl;
    cout << testdim.as_pt() << " pt" << endl;

    testdim.set(2, inch);
    cout << "-- Set to 2 inches --" << endl;
    cout << testdim.as_mm() << " mm" << endl;
    cout << testdim.as_inch() << " inch" << endl;
    cout << testdim.as_pt() << " pt" << endl;

    testdim.set(25.4, "mm");
    cout << "-- Set to 25.4mm --" << endl;
    cout << testdim.as_mm() << " mm" << endl;
    cout << testdim.as_inch() << " inch" << endl;
    cout << testdim.as_pt() << " pt" << endl;

    dim_t testdimB(72, "pt");
    cout << "-- Set to 72pt mm --" << endl;
    cout << testdimB.as_mm() << " mm" << endl;
    cout << testdimB.as_inch() << " inch" << endl;
    cout << testdimB.as_pt() << " pt" << endl;

    dim_t testdimC(1, mm);
    cout << "-- Set to 1 mm --" << endl;
    cout << testdimC.as_mm() << " mm" << endl;
    cout << testdimC.as_inch() << " inch" << endl;
    cout << testdimC.as_pt() << " pt" << endl;

    return 0;

    string bitmap_file = "E0.pbm";
    string stl_file = "E0.stl";

    TypeBitmap *TBM = new TypeBitmap();
    TBM->load(bitmap_file);
    TBM->export_STL(stl_file);

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