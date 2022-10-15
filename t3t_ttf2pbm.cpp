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
    #include <ctype.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <math.h>
    #include <malloc.h>
    #include <argp.h>
#include <ft2build.h>
#include FT_FREETYPE_H
}

const uint8_t BW_THRESHOLD = 32;

///////////////////////////////////////////////////////////////

const float MM_PER_INCH = 25.4;
const float INCH_PER_PT = 0.013835;

////////////////////////////////////////////////////////////////////

struct {
    dim_t raster_size;
    dim_t body_size;
    std::string font_path;
    std::string pbm_path;
    std::string character;
    int unicode;
} argsopts = { .unicode = 0 };

int parse_options(int ac, char* av[]);
int get_yaml_dim_node(YAML::Node &parent, std::string name, dim_t &target);

int main(int ac, char* av[])
{
    parse_options(ac, av);

    int dpi = 891;//round(1 / argsopts.raster_size.as_inch());

    FT_Library library;
    FT_Face face;

    FT_GlyphSlot slot;
    FT_Error error;

    if (argsopts.font_path.empty()) {
        std::cerr << "ERROR: No font file specified." << std::endl;
        exit(1);
    }

    if (argsopts.character.empty()) {
        if (argsopts.unicode == 0) {
        std::cerr << "ERROR: No character oder unicode specified." << std::endl;
        exit(1);
        }
    }

  error = FT_Init_FreeType(&library); /* initialize library */
  /* error handling omitted */

  error = FT_New_Face(library, argsopts.font_path.c_str(), 0, &face); /* create face object */
  /* error handling omitted */

  // MEASURED FOR CORRECTING
  char MEASURED_CHAR = 'E';
  float ABOVE_CHAR_MM = 0.25;
  //;float CHAR_HEIGHT_MM_MEASURED = 20.28; // should be BODY_SIZE(ca. 25.3mm) - ABOVE_CHAR - BELOW_CHAR
  float BELOW_CHAR_MM = 4.78;

  int ptsize =  round(argsopts.body_size.as_pt());

  error = FT_Set_Char_Size(face, ptsize << 6, 0, dpi, 0); /* set char size */
  /* error handling omitted */
  slot = face->glyph;
  /* load glyph image into the slot (erase previous one) */
  error = FT_Load_Char(face, MEASURED_CHAR, FT_LOAD_RENDER);

  // metrics
  float PX_PER_INCH = dpi;
  float INCH_PER_PX = 1 / PX_PER_INCH;
  float MM_PER_PX = INCH_PER_PX * MM_PER_INCH;
  printf("MM_PER_PX: %f\r\n", MM_PER_PX);
  float body_size_inch = ptsize * INCH_PER_PT; // (body size of lead type)
  float BODY_SIZE_MM = body_size_inch * MM_PER_INCH;
  int char_width_px = slot->bitmap.width;
  int char_height_px = slot->bitmap.rows;
  float body_size_px = body_size_inch * PX_PER_INCH;

  // SCALE CORRECTION
  float CHAR_HEIGHT_MM = BODY_SIZE_MM - ABOVE_CHAR_MM - BELOW_CHAR_MM;
  printf("CHAR_HEIGHT_MM: %f\r\n", CHAR_HEIGHT_MM);
  float CHAR_HEIGHT_UNCORRECTED_MM = char_height_px * MM_PER_PX;
  printf("CHAR_HEIGHT_UNCORRECTED_MM: %f\r\n", CHAR_HEIGHT_UNCORRECTED_MM);
  int CORR_dpi = round(dpi * (CHAR_HEIGHT_MM / CHAR_HEIGHT_UNCORRECTED_MM));
  printf("CORR_dpi: %d\r\n", CORR_dpi);
  int ABOVE_CHAR_PX = (round)(ABOVE_CHAR_MM / MM_PER_PX);
  printf("ABOVE_CHAR_PX (w/ spec dpi): %d\r\n", ABOVE_CHAR_PX);
  int baseline_to_char_top_olddpi = slot->bitmap_top;
  // printf("baseline_to_char_top_olddpi (w/ spec dpi): %d\r\n",baseline_to_char_top_olddpi);
  int baseline_to_char_top_corrdpi = round(slot->bitmap_top * (CHAR_HEIGHT_MM / CHAR_HEIGHT_UNCORRECTED_MM));
  // printf("baseline_to_char_top_corrdpi (w/ corr dpi): %d\r\n",baseline_to_char_top_corrdpi);
  int CORR_TYPETOP_TO_BASELINE_PX = baseline_to_char_top_corrdpi + ABOVE_CHAR_PX;
  printf("CORR_TYPETOP_TO_BASELINE_PX: %d\r\n", CORR_TYPETOP_TO_BASELINE_PX);

  // corrected load
  error = FT_Set_Char_Size(face, ptsize << 6, 0, CORR_dpi, 0); /* set char size */
  slot = face->glyph;
  if (!argsopts.character.empty())
    error = FT_Load_Char(face, argsopts.character[0], FT_LOAD_RENDER);
  else
    error = FT_Load_Char(face,argsopts.unicode, FT_LOAD_RENDER);
  char_width_px = slot->bitmap.width;
  char_height_px = slot->bitmap.rows;

  // metrics2
  int ascender_scaled = (int)(face->size->metrics.ascender >> 6);
  int descender_scaled = (int)(face->size->metrics.descender >> 6);
  int ascdesc_size_scaled = (int)((face->size->metrics.ascender) >> 6) + 1 - (int)((face->size->metrics.descender) >> 6);

  int advanceX_px = (int)(slot->advance.x) >> 6; // round with .6 digits for accuracy? Probably fudged later anyway
  int set_width_px = advanceX_px;

  int body_size_scaled = ascdesc_size_scaled;
  float PX_PER_SCALED = body_size_px / body_size_scaled;
  float ascender_px = ascender_scaled * PX_PER_SCALED;
  float descender_px = descender_scaled * PX_PER_SCALED;

  int char_left_start = slot->bitmap_left;
  // int char_top_start = round(ascender_px - slot->bitmap_top); // old, unscaled
  int char_top_start = CORR_TYPETOP_TO_BASELINE_PX - slot->bitmap_top; // corrected stuff



    TypeBitmap TBM((uint32_t)set_width_px /*based on corrected dpi, advanceX*/,
                             body_size_px /*based on uncorrected dpi, ptsize*/);

    TBM.pasteGlyph((uint8_t *)(slot->bitmap.buffer), \
                    char_width_px, char_height_px, \
                    char_top_start, char_left_start);

    TBM.threshold(BW_THRESHOLD);
    TBM.mirror();
    TBM.store(argsopts.pbm_path);

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

        if (config["font file"]) {
            argsopts.font_path = config["font file"].as<std::string>();        
        }

        if (config["character"]) {
            argsopts.character = config["character"].as<std::string>();        
        }

        if (config["unicode"]) {
            argsopts.unicode = config["unicode"].as<int>();        
        }

    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

