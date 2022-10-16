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


extern "C" {
    #include <ft2build.h>
    #include FT_FREETYPE_H
}

inline float i26_6_to_float(uint32_t in)
{
    return (float)(in >> 6) + (float)(in & 0x3f)/64;
}


const uint8_t BW_THRESHOLD = 32;


struct {
    dim_t raster_size;
    dim_t body_size;
    std::string font_path;
    std::string pbm_path;
    std::string character;
    std::vector<uint32_t> characters;
    int unicode;

    std::string ref_char;
    dim_t above_ref_char;
    dim_t below_ref_char;


} argsopts = { .unicode = 0 };


int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);


int main(int ac, char* av[])
{
    parse_options(ac, av);

    FT_Library library;
    FT_Face face;

    FT_GlyphSlot slot;
    FT_Error error;

    if (argsopts.font_path.empty()) {
        std::cerr << "ERROR: No font file specified." << std::endl;
        exit(1);
    }

    if (argsopts.characters.empty()) {
        std::cerr << "ERROR: No characters oder unicodes specified." << std::endl;
        exit(1);
    }

    error = FT_Init_FreeType(&library); /* initialize library */
    /* error handling omitted */

    error = FT_New_Face(library, argsopts.font_path.c_str(), 0, &face); /* create face object */
    /* error handling omitted */


    float dpi = (1 / argsopts.raster_size.as_inch());
    int ptsize =  round(argsopts.body_size.as_pt());

    error = FT_Set_Char_Size(face, ptsize << 6, 0, int(round(dpi)), 0); /* set char size */
    slot = face->glyph;
    /* load glyph image into the slot (erase previous one) */
    error = FT_Load_Char(face, argsopts.ref_char[0], FT_LOAD_RENDER);

    float body_size_px = argsopts.body_size.as_inch() * dpi;

    // metrics for ttf reference glyph at intended dpi
    int glyph_width_px = slot->bitmap.width;
    int glyph_height_px = slot->bitmap.rows;




    // SCALE CORRECTION
    dim_t lead_glyph_height(argsopts.body_size.as_mm()
                            - argsopts.above_ref_char.as_mm()
                            - argsopts.below_ref_char.as_mm(),mm);
    dim_t truetype_glyph_height(glyph_height_px / dpi, inch);

    float ttf_to_lead_scaleup = lead_glyph_height.as_mm() / truetype_glyph_height.as_mm();
        
    int refchar_ascender_px = slot->bitmap_top;
    float upscaled_ascender_px = refchar_ascender_px * ttf_to_lead_scaleup;
    
    int scaledup_dpi = int(round(dpi * ttf_to_lead_scaleup));
    int typetop_to_baseline_px = int(round( upscaled_ascender_px +(argsopts.above_ref_char.as_inch() * dpi) ) );

    printf("Glyph height in lead (mm): %f\r\n", lead_glyph_height.as_mm());
    printf("Glyph height in TrueType (mm): %f\r\n", truetype_glyph_height.as_mm());
    printf("dpi scaled up for lead-size glyph: %d\r\n", scaledup_dpi);
    printf("typetop_to_baseline_px: %d\r\n", typetop_to_baseline_px);


        // scaled up Glyph load
    error = FT_Set_Char_Size(face, ptsize << 6, 0, scaledup_dpi, 0); /* set char size */
    slot = face->glyph;


    for(int i; i<argsopts.characters.size(); i++) {

        uint32_t current_char = argsopts.characters[i];
        error = FT_Load_Char(face, current_char, FT_LOAD_RENDER);

        // glyph size
        glyph_width_px = slot->bitmap.width;
        glyph_height_px = slot->bitmap.rows;

        // type size - height stays (pt size), width based on scaled-up glyph
        float advanceX_px = i26_6_to_float(slot->advance.x);
        int set_width_px = int(round(advanceX_px));

        int char_left_start = slot->bitmap_left;
        int char_top_start = typetop_to_baseline_px - slot->bitmap_top; // corrected stuff



        TypeBitmap TBM((uint32_t)set_width_px /*based on scaled-up dpi (advanceX) */,
                                body_size_px /*based on uncorrected dpi (ptsize) */);

        TBM.pasteGlyph((uint8_t *)(slot->bitmap.buffer), \
                        glyph_width_px, glyph_height_px, \
                        char_top_start, char_left_start);

        TBM.threshold(BW_THRESHOLD);
        TBM.mirror();
        std::string output_path;
        if (current_char < 0x80) {
            char asciistring[6];
            sprintf(asciistring,"%c.pbm",current_char);
            output_path = std::string(asciistring);
        }
        else {
            char hexstring[11];
            sprintf(hexstring,"U+%04x.pbm",current_char);
            output_path = std::string(hexstring);
        }
        TBM.store(output_path);
            
        //TBM.store(argsopts.pbm_path);
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
            ("pbm,p", bpo::value<std::string>(&argsopts.pbm_path), "specify output PBM path")
            //("font,f", bpo::value<std::string>(&argsopts.font_path), "specify input font path")
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

        YAML::Node config = YAML::Load(yaml_config);

        get_yaml_dim_node(config, "raster size", argsopts.raster_size);
        get_yaml_dim_node(config, "body size", argsopts.body_size);
        get_yaml_dim_node(config, "space above refchar", argsopts.above_ref_char);
        get_yaml_dim_node(config, "space below refchar", argsopts.below_ref_char);

        if (config["font file"]) {
            argsopts.font_path = config["font file"].as<std::string>();
        }

        if (config["reference character"]) {
            argsopts.ref_char = config["reference character"].as<std::string>();
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

    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

