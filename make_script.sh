gcc -L/usr/local/lib -Wl,--no-as-needed -lfreetype -lm -I/home/esr/freetype/include/ -o typegen typegen.c

g++ -o yamltest -I/usr/local/include/yaml-cpp/ yamltest.cpp -lyaml-cpp

g++  -std=c++2a -o pbm2stl_type   TypeBitmap.cpp pbm2stl_type.cpp   -lyaml-cpp   -I/usr/local/include/yaml-cpp/ 

