cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)


#add_subdirectory(src)
project(typebitmap VERSION 0.1)
set (SOURCES src/TypeBitmap.cpp)
include_directories(./include/)
add_library(typebitmap STATIC ${SOURCES})


project(typebitmap VERSION 0.1)
set (SOURCES src/AppLog.cpp)
include_directories(./include/)
add_library(applog STATIC ${SOURCES})


project(t3t_image2pbm VERSION 0.1)
add_executable(t3t_image2pbm src/t3t_image2pbm.cpp src/t3t_support_types.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/ /home/esr/freetype/include/)
target_link_libraries(t3t_image2pbm yaml-cpp m freetype boost_program_options typebitmap applog)

project(t3t_ttf2pbm VERSION 0.1)
add_executable(t3t_ttf2pbm src/t3t_ttf2pbm.cpp src/t3t_support_types.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/ /home/esr/freetype/include/)
target_link_libraries(t3t_ttf2pbm yaml-cpp m freetype boost_program_options typebitmap applog)

project(t3t_pbm2stl VERSION 0.1)
add_executable(t3t_pbm2stl src/t3t_pbm2stl.cpp src/t3t_support_types.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/)
target_link_libraries(t3t_pbm2stl yaml-cpp boost_program_options typebitmap applog)

project(t3t_text_composer VERSION 0.1)
add_executable(t3t_text_composer src/t3t_text_composer.cpp src/t3t_support_types.cpp src/PGMbitmap.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/)
target_link_libraries(t3t_text_composer yaml-cpp boost_program_options applog)

project(t3t_STLcompiler VERSION 0.1)
add_executable(t3t_STLcompiler src/t3t_STLcompiler.cpp src/t3t_support_types.cpp)
include_directories(./include/ /usr/local/include/yaml-cpp/)
target_link_libraries(t3t_STLcompiler yaml-cpp boost_program_options applog)
