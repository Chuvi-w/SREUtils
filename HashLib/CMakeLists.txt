cmake_minimum_required(VERSION 3.8)

FILE(GLOB OpenSSLHash_Src OpenSSLHash/*.cpp OpenSSLHash/*.c)
FILE(GLOB OpenSSLHash_Hdr OpenSSLHash_Src/*.h)
FILE(GLOB HashLib_Src *.cpp *.c)
FILE(GLOB HashLib_h *.h)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(STATIC_LIB)
add_library(HashLib STATIC ${OpenSSLHash_Src} ${HashLib_Src} ${OpenSSLHash_Hdr} ${HashLib_h})
else()
add_library(HashLib SHARED ${OpenSSLHash_Src} ${HashLib_Src} ${OpenSSLHash_Hdr} ${HashLib_h})
endif()

source_group("Header Files" FILES ${HashLib_h})
source_group("Source Files" FILES ${HashLib_Src})
source_group("Header Files\\OpenSSLHash" FILES ${OpenSSLHash_Hdr})
source_group("Source Files\\OpenSSLHash" FILES ${OpenSSLHash_Src})
FixStaticLinking()

