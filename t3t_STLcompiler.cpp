#include "yaml.h"
#include "t3t_support_types.h"
#include <iostream>
#include <fstream>
using namespace std;


struct STLfile {
    std::string filename;
    uint32_t tri_count;
    stl_tri_t *triangles;
    float highX, lowX, highY, lowY, highZ, lowZ;
};


int main()
{

    YAML::Node config = YAML::LoadFile("STLcompile.yaml");

    // TODO: Error checking
    float platformX = config["platform size X"].as<float>();
    float platformY = config["platform size Y"].as<float>();
    float gapX = config["gap X"].as<float>();
    float gapY = config["gap Y"].as<float>();

    uint32_t type_count_X = 0;
    uint32_t type_count_Y = 0;

    float posX = 0.0;
    float posY = 0.0;

    uint32_t compiled_tri_cnt = 0;

    STLfile inputSTL;

    // open output file
    std::ofstream stl_out("compiled.stl", std::ios::binary);
    if (!stl_out.is_open()) {
        std::cerr << "ERROR: Could not open STL file for writing." << std::endl;
        return -1;
    }    

    // 80 byte header - content anything but "solid" (would indicated ASCII encoding)
    for (int i = 0; i < 80; i++)
        stl_out.put('x');

    stl_out.write((const char*)&compiled_tri_cnt, 4); // space for number of triangles


    std::vector<std::string> structure;

    for(int i=0; i<config["structure"].size(); i++)
        structure.push_back(config["structure"][i].as<std::string>());

    for(int i=0; i<structure.size(); i++) {
        cout << structure[i] << endl;

        std::string entry;
        std::vector<std::string> line;

        for(int j=0; j<structure[i].size(); j++) {
            char c = structure[i][j];
            if (c==' ') {
                if (!entry.empty()) {
                    line.push_back(entry);
                    entry.clear();
                }
            }
            else
                entry += c;
        }
        if (!entry.empty()) {
            line.push_back(entry);
            entry.clear();
        }

        uint32_t max_line_y = 0;

        // line items
        for(int k=0; k<line.size(); k++) {

            inputSTL.filename = line[k] + ".stl";
            ifstream stl_in(inputSTL.filename) ;
            if (!stl_in.is_open()) {
                std::cerr << "ERROR: Could not open STL file for reading." << std::endl;
                continue;
            }
            stl_in.seekg(80);
            stl_in.read((char*)&inputSTL.tri_count, 4);

            inputSTL.triangles = (stl_tri_t*)malloc(sizeof(stl_tri_t)*inputSTL.tri_count);
            if (inputSTL.triangles) {
                for(int m=0; m<inputSTL.tri_count; m++) {
                    stl_in.read((char*)&(inputSTL.triangles[m]), 50);
                }
                stl_in.close();
            }
            else {
                stl_in.close();
                continue;
            }
            inputSTL.highX = inputSTL.triangles[0].V1x; inputSTL.lowX = inputSTL.triangles[0].V1x;
            inputSTL.highY = inputSTL.triangles[0].V1y; inputSTL.lowY = inputSTL.triangles[0].V1y;
            inputSTL.highZ = inputSTL.triangles[0].V1z; inputSTL.lowZ = inputSTL.triangles[0].V1z;

            for(int m=0; m<inputSTL.tri_count; m++) {
//cout << "V1 " << inputSTL.triangles[m].V1x << "/" << inputSTL.triangles[m].V1y << "/" << inputSTL.triangles[m].V1z << endl;
//cout << "V2 " << inputSTL.triangles[m].V2x << "/" << inputSTL.triangles[m].V2y << "/" << inputSTL.triangles[m].V2z << endl;
//cout << "V3 " << inputSTL.triangles[m].V3x << "/" << inputSTL.triangles[m].V3y << "/" << inputSTL.triangles[m].V3z << endl << endl;

                if (inputSTL.triangles[m].V1x > inputSTL.highX) inputSTL.highX = inputSTL.triangles[m].V1x;
                if (inputSTL.triangles[m].V1x <  inputSTL.lowX)  inputSTL.lowX = inputSTL.triangles[m].V1x;
                if (inputSTL.triangles[m].V2x > inputSTL.highX) inputSTL.highX = inputSTL.triangles[m].V2x;
                if (inputSTL.triangles[m].V2x <  inputSTL.lowX)  inputSTL.lowX = inputSTL.triangles[m].V2x;
                if (inputSTL.triangles[m].V3x > inputSTL.highX) inputSTL.highX = inputSTL.triangles[m].V3x;
                if (inputSTL.triangles[m].V3x <  inputSTL.lowX)  inputSTL.lowX = inputSTL.triangles[m].V3x;

                if (inputSTL.triangles[m].V1y > inputSTL.highY) inputSTL.highY = inputSTL.triangles[m].V1y;
                if (inputSTL.triangles[m].V1y <  inputSTL.lowY)  inputSTL.lowY = inputSTL.triangles[m].V1y;
                if (inputSTL.triangles[m].V2y > inputSTL.highY) inputSTL.highY = inputSTL.triangles[m].V2y;
                if (inputSTL.triangles[m].V2y <  inputSTL.lowY)  inputSTL.lowY = inputSTL.triangles[m].V2y;
                if (inputSTL.triangles[m].V3y > inputSTL.highY) inputSTL.highY = inputSTL.triangles[m].V3y;
                if (inputSTL.triangles[m].V3y <  inputSTL.lowY)  inputSTL.lowY = inputSTL.triangles[m].V3y;

                if (inputSTL.triangles[m].V1z > inputSTL.highZ) inputSTL.highZ = inputSTL.triangles[m].V1z;
                if (inputSTL.triangles[m].V1z <  inputSTL.lowZ)  inputSTL.lowZ = inputSTL.triangles[m].V1z;
                if (inputSTL.triangles[m].V2z > inputSTL.highZ) inputSTL.highZ = inputSTL.triangles[m].V2z;
                if (inputSTL.triangles[m].V2z <  inputSTL.lowZ)  inputSTL.lowZ = inputSTL.triangles[m].V2z;
                if (inputSTL.triangles[m].V3z > inputSTL.highZ) inputSTL.highZ = inputSTL.triangles[m].V3z;
                if (inputSTL.triangles[m].V3z <  inputSTL.lowZ)  inputSTL.lowZ = inputSTL.triangles[m].V3z;
            }
            
            /*
            cout << "X " << inputSTL.lowX << ".." << inputSTL.highX;
            cout << "   Y " << inputSTL.lowY << ".." << inputSTL.highY;
            cout << "   Z " << inputSTL.lowZ << ".." << inputSTL.highZ;
            cout << endl;
            */

            float offsetX = posX -(inputSTL.lowX);
            float offsetY = posY -(inputSTL.lowY);
            float offsetZ = -(inputSTL.lowZ);

            for(int m=0; m<inputSTL.tri_count; m++) {
                inputSTL.triangles[m].V1x += offsetX;
                inputSTL.triangles[m].V2x += offsetX;
                inputSTL.triangles[m].V3x += offsetX;

                inputSTL.triangles[m].V1y += offsetY;
                inputSTL.triangles[m].V2y += offsetY;
                inputSTL.triangles[m].V3y += offsetY;

                inputSTL.triangles[m].V1z += offsetZ;
                inputSTL.triangles[m].V2z += offsetZ;
                inputSTL.triangles[m].V3z += offsetZ;
            }

            for(int m=0; m<inputSTL.tri_count; m++) {
                stl_out.write((const char*)&(inputSTL.triangles[m]), 50);
            }
            compiled_tri_cnt += inputSTL.tri_count;

            free(inputSTL.triangles);

            posX += (inputSTL.highX-inputSTL.lowX);
            posX += gapX;
        }

        if ((inputSTL.highY-inputSTL.lowY) > max_line_y)
            max_line_y = (inputSTL.highY-inputSTL.lowY);

        // ADD Y- POS, set X = 0;
        posY -= max_line_y;
        posY -= gapY;
        posX = 0.0;
    }

    stl_out.seekp(80);
    stl_out.write((const char*)&compiled_tri_cnt, 4);
    stl_out.close();
    
    stl_out.close();
    return 0;
}