cmake_minimum_required (VERSION 3.9)
project (raytracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-g -Wall -Wextra -Wpedantic -Wl,--stack,16777216)

find_package(GTest REQUIRED)

add_executable(geometry_test geometry/geometry_test.cc math/math.cc geometry/geometry.cc)
target_link_libraries(geometry_test GTest::gtest GTest::gtest_main)

add_executable(math_test math/math_test.cc math/math.cc)
target_link_libraries(math_test GTest::gtest GTest::gtest_main)

add_executable(raytracer raytracer.cc math/math.cc view/window.cc view/camera.cc view/viewport.cc utility/color.cc world/objects/material.cc utility/light.cc geometry/geometry.cc world/world.cc)
find_package(SDL2 REQUIRED)
target_include_directories(raytracer PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(raytracer ${SDL2_LIBRARIES})