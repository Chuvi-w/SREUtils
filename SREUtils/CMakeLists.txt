cmake_minimum_required(VERSION 3.8)


FILE(GLOB SREUtils_Src *.cpp *.c)
FILE(GLOB SREUtils_hdr *.h)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(STATIC_LIB)
add_library(SREUtils STATIC ${SREUtils_Src} ${SREUtils_hdr})
else()
add_library(SREUtils SHARED ${SREUtils_Src} ${SREUtils_hdr})
endif()

include_directories("${CMAKE_SOURCE_DIR}/HashLib/")
source_group("Header Files" FILES ${SREUtils_hdr})
source_group("Source Files" FILES ${SREUtils_Src})

target_link_libraries(SREUtils HashLib)
FixStaticLinking()
