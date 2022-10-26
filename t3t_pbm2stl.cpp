#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <boost/format.hpp> 
#include <boost/program_options.hpp> 
#include <sstream>

using namespace std;
namespace bpo = boost::program_options;


struct {
    dim_t type_height; 
    dim_t depth_of_drive;

    dim_t raster_size;
    dim_t layer_height;

    reduced_foot_mode foot_mode;
    dim_t reduced_foot_XY;
    dim_t reduced_foot_Z;

    std::string pbm_path;
    std::string stl_path;
    std::string obj_path;

    std::vector<uint32_t> characters;

    float Zshrink_pct;

} argsopts = { .foot_mode = none, .Zshrink_pct = 0 };


int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);


int main(int ac, char* av[])
{
    parse_options(ac, av);

    float UVstretchZ = (float)100 / ((float)100 + argsopts.Zshrink_pct);
    cout << "Z stretch to compensate UV shrinking: " << UVstretchZ << endl;

    for(int i=0; i<argsopts.characters.size(); i++) {

        uint32_t current_char = argsopts.characters[i];

        std::string pbm_path, stl_path, obj_path;
        if (current_char < 0x80) {
            char asciistring[6];
            sprintf(asciistring,"%c.pbm",current_char);
            pbm_path = std::string(asciistring);
            sprintf(asciistring,"%c.stl",current_char);
            stl_path = std::string(asciistring);
            sprintf(asciistring,"%c.obj",current_char);
            obj_path = std::string(asciistring);
        }
        else {
            char hexstring[11];
            sprintf(hexstring,"U+%04x.pbm",current_char);
            pbm_path = std::string(hexstring);
            sprintf(hexstring,"U+%04x.stl",current_char);
            stl_path = std::string(hexstring);
            sprintf(hexstring,"U+%04x.obj",current_char);
            obj_path = std::string(hexstring);
        }

        TypeBitmap *TBM = new TypeBitmap();
        TBM->set_type_parameters(argsopts.type_height,
                                argsopts.depth_of_drive,
                                argsopts.raster_size,
                                argsopts.layer_height);

        TBM->load(pbm_path);
        TBM->export_STL(stl_path, argsopts.foot_mode,
                        argsopts.reduced_foot_XY, argsopts.reduced_foot_Z,
                        UVstretchZ);
/*
        TBM->export_OBJ(obj_path, argsopts.foot_mode,
                        argsopts.reduced_foot_XY, argsopts.reduced_foot_Z,
                        UVstretchZ);
                        */

     
                                        
    }
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
    std::vector<std::string> yaml_paths;

    try {

        bpo::options_description desc("t3t_pbm2stl: Command-line options and arguments");
        desc.add_options()
            ("help", "produce this help message")
            ("pbm,p", bpo::value<std::string>(&argsopts.pbm_path), "specify input PBM path")
            ("stl,s", bpo::value<std::string>(&argsopts.stl_path), "specify output STL path")
            ("obj,o", bpo::value<std::string>(&argsopts.obj_path), "specify output OBJ path")
            ("yaml,y", bpo::value< vector<string> >(&yaml_paths), "specify YAML configuration file(s)")
        ;

        bpo::variables_map vm;        

        bpo::positional_options_description posopt;
        posopt.add("yaml", -1);
        bpo::store(bpo::command_line_parser(ac, av).
          options(desc).positional(posopt).run(), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }

        for (string& s: yaml_paths) {
            if (!s.empty() && !s.ends_with(".yaml"))
                s.append(".yaml");
        }

        if (yaml_paths.empty() && std::filesystem::exists("config.yaml"))
            yaml_paths.push_back("config.yaml");

        if (!argsopts.pbm_path.empty() && !argsopts.pbm_path.ends_with(".pbm"))
            argsopts.pbm_path.append(".pbm");

        if (!argsopts.stl_path.empty() && !argsopts.stl_path.ends_with(".stl"))
            argsopts.stl_path.append(".stl");

        string yaml_config;

        for (string& s: yaml_paths) {
            if (filesystem::exists(s)) {
                    ifstream yfile(s);
                    while(!yfile.eof()) {
                        string buf;
                        getline(yfile, buf);
                        yaml_config += buf;
                        yaml_config += "\n";
                    }
                    yaml_config += "\n";
            }
        }
                 // TODO: Commandline arg

        YAML::Node config = YAML::Load(yaml_config);

        get_yaml_dim_node(config, "type height", argsopts.type_height);
        get_yaml_dim_node(config, "depth of drive", argsopts.depth_of_drive);
        get_yaml_dim_node(config, "raster size", argsopts.raster_size);
        get_yaml_dim_node(config, "layer height", argsopts.layer_height);
        get_yaml_dim_node(config, "reduced foot XY", argsopts.reduced_foot_XY);
        get_yaml_dim_node(config, "reduced foot Z", argsopts.reduced_foot_Z);

        if (config["reduced foot mode"]) {
            string foot_mode_str = config["reduced foot mode"].as<std::string>();
            if (foot_mode_str=="bevel")
                argsopts.foot_mode = bevel;
            else if (foot_mode_str=="step")
                argsopts.foot_mode = step;
            else
                argsopts.foot_mode = none;
        }

        if (config["characters"]) {
            std::string characters = config["characters"].as<std::string>();
            for(int i=0; i< characters.size(); i++)
                argsopts.characters.push_back((uint32_t)characters[i]);
        }

        if (config["unicode"]) {
            for(int i=0; i<  config["unicode"].size(); i++)
                argsopts.characters.push_back((uint32_t)config["unicode"][i].as<int>());
        }

        if (config["Zshrink_pct"]) {
            argsopts.Zshrink_pct = config["Zshrink_pct"].as<float>();
        }

    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
