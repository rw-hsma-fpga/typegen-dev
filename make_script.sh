#!/bin/bash

gcc -L/usr/local/lib -Wl,--no-as-needed -lfreetype -lm -I/home/esr/freetype/include/ -o typegen typegen.c



g++  -std=c++2a -o t3t_pbm2stl  \
     t3t_support_types.cpp TypeBitmap.cpp t3t_pbm2stl.cpp  \
     -lyaml-cpp   -I/usr/local/include/yaml-cpp/  \
     -lboost_program_options

g++  -std=c++2a -o t3t_ttf2pbm  \
     t3t_support_types.cpp TypeBitmap.cpp t3t_ttf2pbm.cpp  \
     -lyaml-cpp   -I/usr/local/include/yaml-cpp/  \
     -lfreetype   -lm -I/home/esr/freetype/include/  \
     -lboost_program_options
