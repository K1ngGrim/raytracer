cmake_minimum_required (VERSION 3.9)
project (raytracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-g -O2 -Wall -Wextra -Wpedantic -Wl,--stack,16777216)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(geometry_test geometry/geometry_test.cc math/math.cc geometry/geometry.cc)
target_link_libraries(geometry_test GTest::gtest GTest::gtest_main)

add_executable(math_test math/math_test.cc math/math.cc)
target_link_libraries(math_test GTest::gtest GTest::gtest_main)

add_executable(raytracer raytracer.cc math/math.cc view/window.cc view/camera.cc view/viewport.cc utility/color.cc utility/bmp.cc utility/obj_loader.cc world/objects/material.cc utility/light.cc geometry/geometry.cc world/world.cc)
target_link_libraries(raytracer Threads::Threads)

# GPU-Rendering mit Metal (nur macOS): der Shader gpu/raytrace.metal wird als Text in das Programm eingebettet
# und zur Laufzeit kompiliert, dafuer wird kein Xcode benoetigt
if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED True)

    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/gpu/raytrace.metal METAL_SOURCE)
    file(WRITE ${CMAKE_BINARY_DIR}/generated/metal_source.h.tmp "static const char *const kMetalSource = R\"MSL(${METAL_SOURCE})MSL\";\n")
    configure_file(${CMAKE_BINARY_DIR}/generated/metal_source.h.tmp ${CMAKE_BINARY_DIR}/generated/metal_source.h COPYONLY)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS gpu/raytrace.metal)

    target_sources(raytracer PRIVATE gpu/metal_renderer.mm)
    set_source_files_properties(gpu/metal_renderer.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)
    target_include_directories(raytracer PRIVATE ${CMAKE_BINARY_DIR}/generated)
    target_compile_definitions(raytracer PRIVATE USE_METAL)
    target_link_libraries(raytracer "-framework Metal" "-framework Foundation")
endif()
