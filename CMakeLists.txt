cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(t3t_ttf2pbm VERSION 0.1)
add_executable(t3t_ttf2pbm t3t_ttf2pbm.cpp t3t_support_types.cpp TypeBitmap.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/ /home/esr/freetype/include/)
target_link_libraries(t3t_ttf2pbm yaml-cpp m freetype boost_program_options)

project(t3t_pbm2stl VERSION 0.1)
add_executable(t3t_pbm2stl t3t_pbm2stl.cpp t3t_support_types.cpp TypeBitmap.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/)
target_link_libraries(t3t_pbm2stl yaml-cpp boost_program_options)

project(t3t_STLcompiler VERSION 0.1)
add_executable(t3t_STLcompiler t3t_STLcompiler.cpp t3t_support_types.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/)
target_link_libraries(t3t_STLcompiler yaml-cpp boost_program_options)
