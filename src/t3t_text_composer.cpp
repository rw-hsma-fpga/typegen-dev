#include "yaml.h"
#include "PGMbitmap.h"
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
    std::string pbm_path;
    std::string work_path;
        bool create_work_path;

    std::vector<uint32_t> characters;

    std::vector<std::vector<uint32_t>> text;

} opts = { .create_work_path = false };


int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);


int main(int ac, char* av[])
{
    parse_options(ac, av);

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

    // begin temp
    std::string lines[3] = {"HelloWorld","Screwyou","Loveunknowncoder"};

    for(int i=0; i<3; i++) {
        std::vector<uint32_t> vec;
        opts.text.push_back(vec);
        for (int j=0; j<lines[i].size(); j++) {
            opts.text[i].push_back((uint32_t)lines[i][j]);
        }
    }
    // end temp

    for(int i=0; i<opts.text.size(); i++) {
        for (int j=0; j<opts.text[i].size(); j++) {
            std::cout << (char)opts.text[i][j];
        }
        std::cout << std::endl;
    }

    PGMbitmap BM(opts.work_path + "A.pbm");
    BM.storePGM(opts.work_path + "A_grey.pgm");

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

        bpo::options_description desc("t3t_text_composer: Command-line options and arguments");
        desc.add_options()
            ("help", "produce this help message")
            ("pgm,p", bpo::value<std::string>(&opts.pbm_path), "specify output PGM path")
            ("workdir,w", bpo::value<std::string>(&opts.work_path), "working directory (overrides YAML)")            
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

        if (opts.work_path.empty()) {
            if (config["working directory"]) {
                if (config["working directory"]["path"])
                    opts.work_path = config["working directory"]["path"].as<std::string>();
                if (config["working directory"]["create"])
                    opts.create_work_path = config["working directory"]["create"].as<bool>();
            }
        }

        if (config["characters"]) {
            YAML::Node chars = config["characters"];

            if (chars["ASCII"]) {
                std::string characters = chars["ASCII"].as<std::string>();
                for(int i=0; i< characters.size(); i++) {
                    //std::cout << "Unicode in config: " << (uint32_t)characters[i] << std::endl;
                    opts.characters.push_back((uint32_t)characters[i]);

                }
            }

            if (chars["unicode"]) {
                for(int i=0; i<  chars["unicode"].size(); i++) {
                    uint32_t unicode = chars["unicode"][i].as<unsigned int>();
                    //std::cout << "Unicode in config: " << unicode << std::endl;
                    opts.characters.push_back(unicode);
                }
            }
        }

    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

