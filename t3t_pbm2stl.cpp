#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <boost/format.hpp> 
#include <boost/program_options.hpp> 

using namespace std;
namespace bpo = boost::program_options;


struct {
    dim_t type_height; 
    dim_t depth_of_drive;
    //bool beveled_foot;
    dim_t raster_size;
    dim_t layer_height;
    dim_t beveled_foot_depth;
    std::string yaml_path;
    std::string pbm_path;
    std::string stl_path;
} argsopts;// = { .beveled_foot = false };


int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);


int main(int ac, char* av[])
{
    parse_options(ac, av);

    TypeBitmap *TBM = new TypeBitmap();
    TBM->set_type_parameters(argsopts.type_height,
                             argsopts.depth_of_drive,
                             argsopts.raster_size,
                             argsopts.layer_height,
                             argsopts.beveled_foot_depth);

    TBM->load(argsopts.pbm_path);
    TBM->export_STL(argsopts.stl_path);

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

int parse_options(int ac, char* av[])
{
    try {

        bpo::options_description desc("t3t_pbm2stl: Command-line options and arguments");
        desc.add_options()
            ("help", "produce this help message")
            ("yaml,y", bpo::value<std::string>(&argsopts.yaml_path)->default_value("config.yaml"),
                    "specify YAML configuration file")
            ("pbm,p", bpo::value<std::string>(&argsopts.pbm_path), "specify input PBM path")
            ("stl,s", bpo::value<std::string>(&argsopts.stl_path), "specify output STL path")
        ;

        bpo::variables_map vm;        
        bpo::store(bpo::parse_command_line(ac, av, desc), vm);
        bpo::notify(vm);    

        if (!argsopts.yaml_path.empty() && !argsopts.yaml_path.ends_with(".yaml"))
            argsopts.yaml_path.append(".yaml");
        cout << string(argsopts.yaml_path) << endl;

        if (!argsopts.pbm_path.empty() && !argsopts.pbm_path.ends_with(".pbm"))
            argsopts.pbm_path.append(".pbm");
        cout << string(argsopts.pbm_path) << endl;

        if (!argsopts.stl_path.empty() && !argsopts.stl_path.ends_with(".stl"))
            argsopts.stl_path.append(".stl");
        cout << string(argsopts.stl_path) << endl;

        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }


        YAML::Node config = YAML::LoadFile(argsopts.yaml_path); // TODO: Commandline arg

        get_yaml_dim_node(config, "type height", argsopts.type_height);
        get_yaml_dim_node(config, "depth of drive", argsopts.depth_of_drive);
        get_yaml_dim_node(config, "raster size", argsopts.raster_size);
        get_yaml_dim_node(config, "layer height", argsopts.layer_height);
        get_yaml_dim_node(config, "beveled foot depth", argsopts.beveled_foot_depth);

/*
        if (vm.count("compression")) {
            cout << "Compression level was set to " 
                 << vm["compression"].as<double>() << ".\n";
        } else {
            cout << "Compression level was not set.\n";
        }
        */
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
