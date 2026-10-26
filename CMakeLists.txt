cmake_minimum_required (VERSION 3.9)
project (raytracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-g -Wall -Wextra -Wpedantic -Wl,--stack,16777216)

find_package(GTest REQUIRED)

add_executable(geometry_test geometry/geometry_test.cc math/math.cc geometry/geometry.cc)
target_link_libraries(geometry_test GTest::gtest GTest::gtest_main)

add_executable(math_test math/math_test.cc math/math.cc)
target_link_libraries(math_test GTest::gtest GTest::gtest_main)

add_executable(obj_loader_test utility/obj_loader_test.cc utility/obj_loader.cc math/math.cc geometry/geometry.cc world/world.cc world/objects/material.cc utility/color.cc utility/light.cc)
target_link_libraries(obj_loader_test GTest::gtest GTest::gtest_main)

add_executable(raytracer raytracer.cc math/math.cc view/window.cc view/camera.cc view/viewport.cc utility/color.cc utility/bmp.cc utility/obj_loader.cc world/objects/material.cc utility/light.cc geometry/geometry.cc world/world.cc)