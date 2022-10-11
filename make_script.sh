#!/bin/bash

gcc -L/usr/local/lib -Wl,--no-as-needed -lfreetype -lm -I/home/esr/freetype/include/ -o typegen typegen.c

# g++ -o yamltest -I/usr/local/include/yaml-cpp/ yamltest.cpp -lyaml-cpp

g++  -std=c++2a -o t3t_pbm2stl \
     t3t_support_types.cpp TypeBitmap.cpp t3t_pbm2stl.cpp  \
     -lyaml-cpp   -I/usr/local/include/yaml-cpp/ 
