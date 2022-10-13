#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
using namespace std;


struct {
    dim_t type_height; 
    dim_t depth_of_drive;
    bool beveled_foot;
    dim_t raster_size;
    dim_t layer_height;
    dim_t beveled_foot_depth;
} argsopts = { .beveled_foot = false };

int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);

int main()
{

    YAML::Node config = YAML::LoadFile("config.yaml"); // TODO: Commandline arg

    get_yaml_dim_node(config, "type height", argsopts.type_height);
    get_yaml_dim_node(config, "depth of drive", argsopts.depth_of_drive);
    get_yaml_dim_node(config, "raster size", argsopts.raster_size);
    get_yaml_dim_node(config, "layer height", argsopts.layer_height);
    get_yaml_dim_node(config, "beveled foot depth", argsopts.beveled_foot_depth);

    string bitmap_file = "E0.pbm";
    string stl_file = "E2.stl";


    TypeBitmap *TBM = new TypeBitmap();
    TBM->set_type_parameters(argsopts.type_height,
                             argsopts.depth_of_drive,
                             argsopts.raster_size,
                             argsopts.layer_height,
                             argsopts.beveled_foot_depth);

    TBM->load(bitmap_file);
    TBM->export_STL(stl_file);

    return 0;
}


int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target)
{
    if (parent[name]["value"] && parent[name]["unit"])
        target = dim_t(parent[name]["value"].as<float>(),
                       parent[name]["unit"].as<std::string>());
    else
        return -1;

    return 0;
}

