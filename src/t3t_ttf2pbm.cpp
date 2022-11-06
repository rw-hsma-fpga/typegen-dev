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


extern "C" {
    #include <ft2build.h>
    #include FT_FREETYPE_H
}

inline float i26_6_to_float(uint32_t in)
{
    return (float)(in >> 6) + (float)(in & 0x3f)/64;
}


const uint8_t BW_THRESHOLD = 1;


struct {
    dim_t raster_size;
    dim_t body_size;

    std::string font_path;
    std::string pbm_path;
    std::string work_path;
        bool create_work_path;

    std::vector<uint32_t> characters;

    std::string ref_char;
    dim_t above_ref_char;
    dim_t below_ref_char;

    float XYshrink_pct;

} opts = { .create_work_path = false, .XYshrink_pct = 0 };


int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);


int main(int ac, char* av[])
{
    parse_options(ac, av);

    float UVstretchXY = (float)100 / ((float)100 + opts.XYshrink_pct);
    cout << "XY stretch to compensate UV shrinking: " << UVstretchXY << endl;

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


    FT_Library library;
    FT_Face face;
    FT_GlyphSlot slot;
    FT_Error error;

    if (opts.font_path.empty()) {
        std::cerr << "ERROR: No font file specified." << std::endl;
        exit(1);
    }

    if (opts.characters.empty()) {
        std::cerr << "ERROR: No characters oder unicodes specified." << std::endl;
        exit(1);
    }

    error = FT_Init_FreeType(&library); /* initialize library */
    if (error) {
        std::cout << "Error: FT_Init_FreeType() failed with error " << error << std::endl;
        exit(1);
    }

    error = FT_New_Face(library, opts.font_path.c_str(), 0, &face); /* create face object */
    if (error) {
        std::cout << "Error: FT_New_Face() failed with error " << error << std::endl;
        exit(1);
    }


    float dpi = (1 / (opts.raster_size.as_inch() / UVstretchXY )); /// UVstretchXY
    int ptsize =  round(opts.body_size.as_pt());

    error = FT_Set_Char_Size(face, ptsize << 6, 0, int(round(dpi)), 0); /* set char size */
    if (error) {
        std::cout << "Error: FT_Set_Char_Size() failed with error " << error << std::endl;
        exit(1);
    }
    slot = face->glyph;
    /* load glyph image into the slot (erase previous one) */
    error = FT_Load_Char(face, opts.ref_char[0], FT_LOAD_RENDER);
    if (error) {
        std::cout << "Error: FT_Load_Char() failed with error " << error << std::endl;
        exit(1);
    }

    float body_size_px = opts.body_size.as_inch() * dpi;

    // metrics for ttf reference glyph at intended dpi
    int glyph_width_px = slot->bitmap.width;
    int glyph_height_px = slot->bitmap.rows;




    // SCALE CORRECTION
    dim_t lead_glyph_height(opts.body_size.as_mm()
                            - opts.above_ref_char.as_mm()
                            - opts.below_ref_char.as_mm(),mm);
    dim_t truetype_glyph_height(glyph_height_px / dpi, inch);

    float ttf_to_lead_scaleup = lead_glyph_height.as_mm() / truetype_glyph_height.as_mm();
        
    int refchar_ascender_px = slot->bitmap_top;
    float upscaled_ascender_px = refchar_ascender_px * ttf_to_lead_scaleup;
    
    int scaledup_dpi = int(round(dpi * ttf_to_lead_scaleup));
    int typetop_to_baseline_px = int(round( upscaled_ascender_px +(opts.above_ref_char.as_inch() * dpi) ) );

    printf("Glyph height in lead (mm): %f\r\n", lead_glyph_height.as_mm());
    printf("Glyph height in TrueType (mm): %f\r\n", truetype_glyph_height.as_mm());
    printf("dpi scaled up for lead-size glyph: %d\r\n", scaledup_dpi);
    printf("typetop_to_baseline_px: %d\r\n", typetop_to_baseline_px);


        // scaled up Glyph load
    error = FT_Set_Char_Size(face, ptsize << 6, 0, scaledup_dpi, 0); /* set char size */
    if (error) {
        std::cout << "Error: FT_Set_Char_Size() failed with error " << error << std::endl;
        exit(1);
    }

    slot = face->glyph;



    for(int i=0; i<opts.characters.size(); i++) {

        uint32_t current_char = opts.characters[i];
        error = FT_Load_Char(face, current_char, FT_LOAD_RENDER);
        if (error) {
            std::cout << "Error: FT_Load_Char() failed with error " << error << std::endl;
            exit(1);
        }
        // glyph size
        glyph_width_px = slot->bitmap.width;
        glyph_height_px = slot->bitmap.rows;

        // type size - height stays (pt size), width based on scaled-up glyph
        float advanceX_px = i26_6_to_float(slot->advance.x);
        int set_width_px = int(round(advanceX_px));

        int char_left_start = slot->bitmap_left;
        int char_top_start = typetop_to_baseline_px - slot->bitmap_top; // corrected stuff

        // TEMPORARY
//        TypeBitmap TBM((uint32_t)glyph_width_px, // based on scaled-up dpi (advanceX)
//                                body_size_px); //based on uncorrected dpi (ptsize)
//        TBM.pasteGlyph((uint8_t *)(slot->bitmap.buffer),
//                        glyph_width_px, glyph_height_px,
//                        char_top_start, 0);
        // END TEMPORARY

        TypeBitmap TBM((uint32_t)set_width_px,  // based on scaled-up dpi (advanceX)
                                body_size_px); //based on uncorrected dpi (ptsize)
        TBM.pasteGlyph((uint8_t *)(slot->bitmap.buffer), 
                        glyph_width_px, glyph_height_px, 
                        char_top_start, char_left_start);

        TBM.threshold(BW_THRESHOLD);
        TBM.mirror();
        std::string output_path;
        if (current_char < 0x80) {
            char asciistring[3];
            sprintf(asciistring,"/%c",current_char);
            output_path = opts.work_path + std::string(asciistring) + ".pbm";
        }
        else {
            char hexstring[8]; // TODO: 5-digit Unicode support?
            sprintf(hexstring,"/U+%04x",current_char);
            output_path = opts.work_path + std::string(hexstring) + ".pbm";
        }
        //std::cout << "Storing " << output_path << std::endl;
        TBM.store(output_path);
            
    }

    FT_Done_Face(face);

    FT_Done_FreeType(library);

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
            ("pbm,p", bpo::value<std::string>(&opts.pbm_path), "specify output PBM path")
            //("font,f", bpo::value<std::string>(&opts.font_path), "specify input font path")
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
        get_yaml_dim_node(config, "space above refchar", opts.above_ref_char);
        get_yaml_dim_node(config, "space below refchar", opts.below_ref_char);

        if (config["font file"]) {
            opts.font_path = config["font file"].as<std::string>();
        }

        if (config["working directory"]) {
            if (config["working directory"]["path"])
                opts.work_path = config["working directory"]["path"].as<std::string>();
            if (config["working directory"]["create"])
                opts.create_work_path = config["working directory"]["create"].as<bool>();
        }

        if (config["reference character"]) {
            opts.ref_char = config["reference character"].as<std::string>();
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

