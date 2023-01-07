#include "yaml.h"
#include "TypeBitmap.h"
#include "AppLog.h"
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

        reduced_foot foot;

        std::vector<nick> nicks;

        std::string pbm_path;
        std::string stl_path;
        std::string obj_path;
        std::string work_path;
            bool create_work_path;    

        std::vector<uint32_t> characters;
        std::string ASCII; // for command-line input spec
        uint32_t unicode ; // for command-line input spec; overrides ASCII

        std::vector<std::string> images;

        float XYshrink_pct;
        float UVstretchXY;

        float Zshrink_pct;
        float UVstretchZ;

    } opts = { .create_work_path = false, .unicode = 0, .XYshrink_pct = 0, .Zshrink_pct = 0 };

    std::string make_ASCII_Unicode_string(uint32_t);
    int generate_3D_files(TypeBitmap &TBM, std::string pbm_path, std::string stl_path, std::string obj_path);
    int parse_options(int ac, char* av[]);
    int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);

    AppLog logger("pbm2stl", LOGMASK_NOINFO);
    const std::string version("(v0.5)");


int main(int ac, char* av[])
{
    logger.PRINT() << "t3t_pbm2stl " << version << std::endl;

    parse_options(ac, av);

    opts.UVstretchZ = (float)100 / ((float)100 + opts.Zshrink_pct);
    logger.INFO() << "Z stretch to compensate UV shrinking: " << opts.UVstretchZ << endl;
    opts.UVstretchXY = (float)100 / ((float)100 + opts.XYshrink_pct);

    if (!opts.work_path.empty()) {
        if (!fs::exists(opts.work_path)) {
            if (opts.create_work_path) {
                if (!fs::create_directory(opts.work_path)) {
                    logger.ERROR() << "Creating work directory " << opts.work_path << " failed." << endl;
                    exit(1);
                }            
            }
            else {
                logger.ERROR() << "Specified work directory " << opts.work_path << " does not exist." << endl;
                exit(1);
            }
        }
    }
    else {
        opts.work_path = "./";
    } 

    std::string pbm_path, stl_path, obj_path;
    bool clPBM = false;
    bool clworkpathPBM = false;

    if (opts.unicode) { // overrides PBM specification
        opts.pbm_path = make_ASCII_Unicode_string(opts.unicode) + ".pbm";
    }
    
    if (!opts.pbm_path.empty()) { // single PBM input specified
        if (fs::exists(opts.pbm_path)) {
            clPBM = true;
            pbm_path = opts.pbm_path;
        }
        else if (fs::exists(opts.work_path + opts.pbm_path)) {
            clPBM = true;
            clworkpathPBM = true;
            pbm_path = opts.work_path + opts.pbm_path;
        }
        else {
            logger.ERROR() << "Specified input bitmap file '" << (opts.work_path + opts.pbm_path) << "' does not exist." << endl;
            exit(1);
        }
    }

    if (clPBM) {
        if (clPBM && !opts.stl_path.empty()) {
            if (!clworkpathPBM) {
                stl_path = opts.stl_path;
            }
            else {
                stl_path = opts.work_path + opts.stl_path;
            }
        }

        if (clPBM && !opts.obj_path.empty()) {
            if (!clworkpathPBM) {
                obj_path = opts.obj_path;
            }
            else {
                obj_path = opts.work_path + opts.obj_path;
            }
        }

        if (opts.stl_path.empty() && opts.obj_path.empty()) {
            stl_path = pbm_path.substr(0, pbm_path.size()-4) + ".stl";
            obj_path = pbm_path.substr(0, pbm_path.size()-4) + ".obj";
        }
    }

    TypeBitmap TBM;
    TBM.set_type_parameters(opts.type_height,
                            opts.depth_of_drive,
                            opts.raster_size,
                            opts.layer_height);

    if (clPBM) {
        generate_3D_files(TBM, pbm_path, stl_path, obj_path);
    }
    else {
        for(int i=0; i<opts.characters.size(); i++) {

            uint32_t current_char = opts.characters[i];

            std::string AU_string = make_ASCII_Unicode_string(current_char);

            pbm_path = opts.work_path + AU_string + ".pbm";
            stl_path = opts.work_path + AU_string + ".stl";
            obj_path = opts.work_path + AU_string + ".obj";

            generate_3D_files(TBM, pbm_path, stl_path, obj_path);
        }
        for(int i=0; i<opts.images.size(); i++) {

            pbm_path = opts.work_path + opts.images[i] + ".pbm";
            stl_path = opts.work_path + opts.images[i] + ".stl";
            obj_path = opts.work_path + opts.images[i] + ".obj";

            generate_3D_files(TBM, pbm_path, stl_path, obj_path);
        }
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


int generate_3D_files(TypeBitmap &TBM, std::string pbm_path, std::string stl_path, std::string obj_path)
{
    if (TBM.load(pbm_path)<0)
        return -1;

    if (TBM.generateMesh(opts.foot, opts.nicks, opts.UVstretchXY, opts.UVstretchZ)<0)
        return -1;

    if (!stl_path.empty())
        if (TBM.writeSTL(stl_path)<0)
        return -1;

    if (!obj_path.empty())
        if (TBM.writeOBJ(obj_path)<0)
        return -1;

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
    std::string unicode_arg;

    try {

        bpo::options_description desc("t3t_pbm2stl: Command-line options and arguments");
        desc.add_options()
            ("help", "produce this help message")
            ("unicode,u", bpo::value<std::string>(&unicode_arg), "specify input unicode (overrides other input args)")
            ("ascii,a", bpo::value<std::string>(&opts.ASCII), "specify input ASCII character (overrides input PBM)")
            ("pbm,p", bpo::value<std::string>(&opts.pbm_path), "specify input PBM path (overrides YAML)")
            ("stl,s", bpo::value<std::string>(&opts.stl_path), "specify output STL path (only useful if input specified here)")
            ("obj,o", bpo::value<std::string>(&opts.obj_path), "specify output OBJ path (only useful if input specified here)")
            ("yaml,y", bpo::value< vector<string> >(&yaml_paths), "specify YAML configuration file(s)")
        ;
        bpo::variables_map vm;        

        bpo::positional_options_description posopt;
        posopt.add("yaml", -1);
        bpo::store(bpo::command_line_parser(ac, av).
          options(desc).positional(posopt).run(), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            logger.PRINT() << desc << "\n";
            exit(0);
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

        if (!opts.obj_path.empty() && !opts.obj_path.ends_with(".obj"))
            opts.obj_path.append(".obj");

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

        get_yaml_dim_node(config, "type height", opts.type_height);
        get_yaml_dim_node(config, "depth of drive", opts.depth_of_drive);
        get_yaml_dim_node(config, "raster size", opts.raster_size);
        get_yaml_dim_node(config, "layer height", opts.layer_height);
        get_yaml_dim_node(config, "reduced foot XY", opts.foot.XY);
        get_yaml_dim_node(config, "reduced foot Z", opts.foot.Z);

        // ASCII / Unicode args
        if (opts.ASCII != "")
            opts.unicode = (uint32_t)opts.ASCII[0];
        if (!unicode_arg.empty())
            opts.unicode = std::stoul(unicode_arg, 0, 0);

        // NICKS
        if (config["nicks"]) {
            dim_t nick_scale;
            if (config["nicks"]["scale"]) {
                YAML::Node nicks = config["nicks"];
                get_yaml_dim_node(nicks, "scale", nick_scale);
                if (nicks["segments"]) {
                    YAML::Node nicksegs = nicks["segments"];
                    for (int i=0; i<nicksegs.size(); i++) {
                        std::string nick_type;
                        float z = 0, y = 0;
                        nick current_nick;

                        if (nicksegs[i]["type"])
                            nick_type = nicksegs[i]["type"].as<std::string>();
                        if (nick_type == "flat")
                            current_nick.type = flat;
                        if (nick_type == "triangle")
                            current_nick.type = triangle;
                        if (nick_type == "rect")
                            current_nick.type = rect;
                        if (nick_type == "circle")
                            current_nick.type = circle;

                        if (current_nick.type == nick_undefined) {
                            logger.WARNING() << "No valid nick type specified" << std::endl;
                            continue;
                        }

                        if (nicksegs[i]["z"]) {
                            z = nicksegs[i]["z"].as<float>();
                            current_nick.z = dim_t(z * nick_scale.as_mm(), mm);
                        }
                        if (current_nick.z.as_mm() == 0) {
                            logger.WARNING() << "Nick with no height specified" << std::endl;
                            continue;
                        }
                            
                        if (nicksegs[i]["y"]) {
                            y = nicksegs[i]["y"].as<float>();
                            current_nick.y = dim_t(y * nick_scale.as_mm(), mm);
                        }
                        if ((current_nick.type == rect) && 
                            (current_nick.y.as_mm() == 0) ) {
                            logger.WARNING() << "Rectangular nick with no depth specified" << std::endl;
                            continue;
                        }

                        opts.nicks.push_back(current_nick);
                    }
                }
            }
        }

        // REDUCED FOOT PARAMETERS
        if (config["reduced foot mode"]) {
            string foot_mode_str = config["reduced foot mode"].as<std::string>();
            if (foot_mode_str=="bevel")
                opts.foot.mode = bevel;
            else if (foot_mode_str=="step")
                opts.foot.mode = step;
            else if (foot_mode_str=="supports")
                opts.foot.mode = supports;
            else if (foot_mode_str=="pyramids")
                opts.foot.mode = pyramids;
            else
                opts.foot.mode = no_foot;
        }

        if (opts.foot.mode == pyramids) {
            if (config["pyramid pitch"])
                get_yaml_dim_node(config, "pyramid pitch", opts.foot.pyramid_pitch);
            else
                opts.foot.pyramid_pitch.set(0,mm);

            if (config["pyramid top length"])
                get_yaml_dim_node(config, "pyramid top length", opts.foot.pyramid_top_length);
            else
                opts.foot.pyramid_top_length.set(0,mm);

            if (config["pyramid foot height"])
                get_yaml_dim_node(config, "pyramid foot height", opts.foot.pyramid_foot_height);
            else
                opts.foot.pyramid_foot_height.set(0,mm);

            if (config["pyramid height factor"])
                opts.foot.pyramid_height_factor = config["pyramid height factor"].as<float>();
            else
                opts.foot.pyramid_height_factor = 1.0;

        }

        // TODO: SANITY CHECK FOR TYPE HEIGHT
        float body_bottom_margin_mm = 0.5;
        float body_top_strip_mm = 2.0; // needed to connect cleanly w/ type surface

        float height_sum_mm = body_top_strip_mm + body_bottom_margin_mm
                              + opts.depth_of_drive.as_mm() + opts.foot.Z.as_mm();
        for (int i=0; i<opts.nicks.size(); i++) {
            height_sum_mm += opts.nicks[i].z.as_mm();
        }

        if (height_sum_mm > opts.type_height.as_mm()) {
            logger.ERROR() << "Specified type height components (Depth of drive, margins, nicks, reduced foot)" << std::endl
                      << "       bigger than specified Type Height" << std::endl;
            // TODO: List components
            exit(1);
        }


        // WORKING DIRECTORY
        if (config["working directory"]) {
            if (config["working directory"]["path"])
                opts.work_path = config["working directory"]["path"].as<std::string>();
            if (config["working directory"]["create"])
                opts.create_work_path = config["working directory"]["create"].as<bool>();
        }
        
        // LIST OF TYPE CHARACTERS REQUIRED
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

            if (chars["images"]) {
                for(int i=0; i<  chars["images"].size(); i++) {
                    std::string image = chars["images"][i].as<std::string>();
                    if (image.ends_with(".pbm"))
                        image.erase(image.length()-4);
                    opts.images.push_back(image);
                }
            }
        }

        // SHRINKAGE OBSERVED AND TO BE COMPENSATED FOR
        if (config["XYshrink_pct"]) {
            opts.XYshrink_pct = config["XYshrink_pct"].as<float>();
        }
        if (config["Zshrink_pct"]) {
            opts.Zshrink_pct = config["Zshrink_pct"].as<float>();
        }

    }
    catch(exception& e) {
        logger.ERROR() << e.what() << "\n";
        return 1;
    }
    return 0;
}
