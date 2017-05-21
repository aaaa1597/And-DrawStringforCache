cmake_minimum_required(VERSION 3.4.1)

add_library( testlib SHARED
            src/main/cpp/CppSurfaceView.cpp
            src/main/cpp/DrawString.cpp
        )

#freetype
include_directories(src/main/cpp/freetype/inc)
add_library(freetype271 SHARED IMPORTED)
set_target_properties(freetype271 PROPERTIES IMPORTED_LOCATION ../../../../jniLibs/${ANDROID_ABI}/libfreetype271.so)

#libpng
include_directories(src/main/cpp/libpng)
add_library(testlibpng SHARED IMPORTED)
set_target_properties(testlibpng PROPERTIES IMPORTED_LOCATION ../../../../jniLibs/${ANDROID_ABI}/libtestlibpng.so)

#icu
include_directories(src/main/cpp/icu src/main/cpp/icu/common )
add_library(testicuuc SHARED IMPORTED)
set_target_properties(testicuuc PROPERTIES IMPORTED_LOCATION ../../../../jniLibs/${ANDROID_ABI}/libtesticuuc.so)

find_library( log-lib log )

target_link_libraries( testlib ${log-lib} android EGL GLESv2 m freetype271 testlibpng testicuuc )
