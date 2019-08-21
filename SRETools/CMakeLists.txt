cmake_minimum_required(VERSION 3.9)

add_executable(SRETools SREToolsMain.cpp)
include_directories("${CMAKE_SOURCE_DIR}/HashLib/" "${CMAKE_SOURCE_DIR}/SREUtils/")

target_link_libraries(SRETools HashLib SREUtils)
FixStaticLinking()