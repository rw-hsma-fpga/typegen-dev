#include "yaml.h"
#include "TypeBitmap.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <boost/format.hpp> 
#include <boost/program_options.hpp> 
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

using namespace std;
namespace fs = std::filesystem;
namespace bpo = boost::program_options;




struct {
    dim_t raster_size;
    dim_t body_size;

    std::string font_path;
    std::string image_path;
    std::string work_path;
        bool create_work_path;

    float XYshrink_pct;

} opts = { .create_work_path = false, .XYshrink_pct = 0 };


std::string make_ASCII_Unicode_string(uint32_t unicode);
int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);



int shellcall(std::string cmd, std::string &result) {
    char buffer[4096];
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    return pclose(pipe);
}




int main(int ac, char* av[])
{
    parse_options(ac, av);

    std::string shell_output;

    if (shellcall("which convert", shell_output) != 0) {
        std::cerr << "ERROR: 'convert' tool not found." << std::endl;
        std::cerr << "       Please install ImageMagick package before using t3t_image2pbm" << std::endl;
        exit(1);
    }

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

    float UVstretchXY = (float)100 / ((float)100 + opts.XYshrink_pct);
    cout << "XY stretch to compensate UV shrinking: " << UVstretchXY << endl;

    float dpi = (1 / (opts.raster_size.as_inch() / UVstretchXY )); /// UVstretchXY
    int ptsize =  round(opts.body_size.as_pt());

    uint32_t body_size = uint32_t(round(opts.body_size.as_inch() * dpi));

    std::stringstream convert_call;
    convert_call << "convert" << " " << opts.image_path << " -threshold 50% -resize x" << body_size << " " << opts.image_path << ".pbm";
    std::cout << convert_call.str() << std::endl;

    int shell_call_code;
    shell_call_code = shellcall(convert_call.str(), shell_output);
    if (shell_call_code != 0) {
        std::cerr << "ERROR: Converting " << opts.image_path << "to PBM failed with error code " << shell_call_code << std::endl;
        exit(shell_call_code);
    }

    return 0;
}


std::string make_ASCII_Unicode_string(uint32_t unicode)
{
    if (unicode < 0x80) { // TODO: Special handling for special ASCII chars like space?
        char asciistring[3];
        sprintf(asciistring,"/%c", unicode);
        return std::string(asciistring);
    }
    else {
        char hexstring[8]; // TODO: 5-digit Unicode support?
        sprintf(hexstring,"/U+%04x", unicode);
        return std::string(hexstring);
    }
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

        bpo::options_description desc("t3t_image2pbm: Command-line options and arguments");
        desc.add_options()
            ("help", "produce this help message")
            ("image,i", bpo::value<std::string>(&opts.image_path), "specify input image path")
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
            exit(0);
        }

        for (string& s: yaml_paths) {
            if (!s.empty() && !s.ends_with(".yaml"))
                s.append(".yaml");
        }

        if (yaml_paths.empty() && fs::exists("config.yaml"))
            yaml_paths.push_back("config.yaml");

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

        YAML::Node config = YAML::Load(yaml_config);

        get_yaml_dim_node(config, "raster size", opts.raster_size);
        get_yaml_dim_node(config, "body size", opts.body_size);

        if (config["working directory"]) {
            if (config["working directory"]["path"])
                opts.work_path = config["working directory"]["path"].as<std::string>();
            if (config["working directory"]["create"])
                opts.create_work_path = config["working directory"]["create"].as<bool>();
        }


        if (config["XYshrink_pct"]) {
            opts.XYshrink_pct = config["XYshrink_pct"].as<float>();
        }

    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

