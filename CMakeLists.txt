cmake_minimum_required(VERSION 3.6)
project(art_gallery_problem)

#set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")

find_package(X11)
link_libraries(${X11_LIBRARIES})

include_directories(${X11_INCLUDE_DIR})

set(SOURCE_FILES art-gallery.cpp)
add_executable(art_gallery_problem ${SOURCE_FILES})