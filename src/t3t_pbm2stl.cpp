#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <boost/format.hpp> 
#include <boost/program_options.hpp> 
#include <sstream>

using namespace std;
namespace fs = std::filesystem;
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
    std::string work_path;
        bool create_work_path;    

    std::vector<uint32_t> characters;

    float Zshrink_pct;

} opts = { .foot_mode = none, .create_work_path = false, .Zshrink_pct = 0 };


int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);


int main(int ac, char* av[])
{
    parse_options(ac, av);

    float UVstretchZ = (float)100 / ((float)100 + opts.Zshrink_pct);
    cout << "Z stretch to compensate UV shrinking: " << UVstretchZ << endl;

    if (!opts.work_path.empty()) {
        if (!fs::exists(opts.work_path)) {
            if (opts.create_work_path) {
                if (!fs::create_directory(opts.work_path)) {
                    cerr << "Creating work directory " << opts.work_path << " failed." << endl;
                    exit(1);
                }            
            }
            else {
                cerr << "Specified work directory " << opts.work_path << " does not exist." << endl;
                exit(1);
            }
        }
    }
    else {
        opts.work_path = "./";
    } 

    // TODO command line override of character list on input/output
    for(int i=0; i<opts.characters.size(); i++) {

        uint32_t current_char = opts.characters[i];

        std::string pbm_path, stl_path, obj_path;
        if (current_char < 0x80) {
            char asciistring[3];
            sprintf(asciistring,"/%c",current_char);
            pbm_path = opts.work_path + std::string(asciistring) + ".pbm";
            stl_path = opts.work_path + std::string(asciistring) + ".stl";
            obj_path = opts.work_path + std::string(asciistring) + ".obj";
        }
        else {
            char hexstring[8]; // TODO: 5-digit Unicode support?
            sprintf(hexstring,"/U+%04x",current_char);
            pbm_path = opts.work_path + std::string(hexstring) + ".pbm";
            stl_path = opts.work_path + std::string(hexstring) + ".stl";
            obj_path = opts.work_path + std::string(hexstring) + ".obj";
        }

        TypeBitmap *TBM = new TypeBitmap();
        TBM->set_type_parameters(opts.type_height,
                                opts.depth_of_drive,
                                opts.raster_size,
                                opts.layer_height);

        TBM->load(pbm_path);
        TBM->export_STL(stl_path, opts.foot_mode,
                        opts.reduced_foot_XY, opts.reduced_foot_Z,
                        UVstretchZ);

        TBM->export_OBJ(obj_path, opts.foot_mode,
                        opts.reduced_foot_XY, opts.reduced_foot_Z,
                        UVstretchZ);
                        

     
                                        
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
            ("pbm,p", bpo::value<std::string>(&opts.pbm_path), "specify input PBM path")
            ("stl,s", bpo::value<std::string>(&opts.stl_path), "specify output STL path")
            ("obj,o", bpo::value<std::string>(&opts.obj_path), "specify output OBJ path")
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

        if (yaml_paths.empty() && fs::exists("config.yaml"))
            yaml_paths.push_back("config.yaml");

        if (!opts.pbm_path.empty() && !opts.pbm_path.ends_with(".pbm"))
            opts.pbm_path.append(".pbm");

        if (!opts.stl_path.empty() && !opts.stl_path.ends_with(".stl"))
            opts.stl_path.append(".stl");

        string yaml_config;

        for (string& s: yaml_paths) {
            if (fs::exists(s)) {
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

        get_yaml_dim_node(config, "type height", opts.type_height);
        get_yaml_dim_node(config, "depth of drive", opts.depth_of_drive);
        get_yaml_dim_node(config, "raster size", opts.raster_size);
        get_yaml_dim_node(config, "layer height", opts.layer_height);
        get_yaml_dim_node(config, "reduced foot XY", opts.reduced_foot_XY);
        get_yaml_dim_node(config, "reduced foot Z", opts.reduced_foot_Z);

        if (config["reduced foot mode"]) {
            string foot_mode_str = config["reduced foot mode"].as<std::string>();
            if (foot_mode_str=="bevel")
                opts.foot_mode = bevel;
            else if (foot_mode_str=="step")
                opts.foot_mode = step;
            else
                opts.foot_mode = none;
        }

        if (config["working directory"]) {
            if (config["working directory"]["path"])
                opts.work_path = config["working directory"]["path"].as<std::string>();
            if (config["working directory"]["create"])
                opts.create_work_path = config["working directory"]["create"].as<bool>();
        }
        
        if (config["characters"]) {
            YAML::Node chars = config["characters"];

            if (chars["ASCII"]) {
                std::string characters = chars["ASCII"].as<std::string>();
                for(int i=0; i< characters.size(); i++)
                    opts.characters.push_back((uint32_t)characters[i]);
            }

            if (chars["unicode"]) {
                for(int i=0; i<  chars["unicode"].size(); i++)
                    opts.characters.push_back((uint32_t)chars["unicode"][i].as<int>());
            }
        }


        if (config["Zshrink_pct"]) {
            opts.Zshrink_pct = config["Zshrink_pct"].as<float>();
        }

    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
