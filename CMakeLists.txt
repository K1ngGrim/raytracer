cmake_minimum_required (VERSION 3.10)
project (raytracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-g -Wall -Wextra -Wpedantic)

find_package(GTest REQUIRED)

# math: Vektoren und Farben, geometry: Strahl, Kugel, Dreieck, scene: Objekte, Materialien, Licht und Suchen,
# render: Kamera, Beleuchtung und Schleife ueber die Pixel, io: Wavefront-Import und BMP-Ausgabe
set(MATH_SOURCES math/math.cc math/color.cc)
set(SCENE_SOURCES ${MATH_SOURCES} geometry/geometry.cc scene/material.cc scene/scene_object.cc scene/scene.cc)
set(RENDER_SOURCES ${SCENE_SOURCES} render/camera.cc render/shader.cc render/renderer.cc)

add_executable(math_test math/math_test.cc ${MATH_SOURCES})
target_link_libraries(math_test GTest::gtest GTest::gtest_main)

add_executable(geometry_test geometry/geometry_test.cc math/math.cc geometry/geometry.cc)
target_link_libraries(geometry_test GTest::gtest GTest::gtest_main)

add_executable(scene_test scene/scene_test.cc ${SCENE_SOURCES})
target_link_libraries(scene_test GTest::gtest GTest::gtest_main)

add_executable(render_test render/render_test.cc ${RENDER_SOURCES})
target_link_libraries(render_test GTest::gtest GTest::gtest_main)

add_executable(obj_loader_test io/obj_loader_test.cc io/obj_loader.cc ${SCENE_SOURCES})
target_link_libraries(obj_loader_test GTest::gtest GTest::gtest_main)

add_executable(bmp_test io/bmp_test.cc io/bmp.cc ${MATH_SOURCES})
target_link_libraries(bmp_test GTest::gtest GTest::gtest_main)

add_executable(raytracer raytracer.cc io/obj_loader.cc io/bmp.cc ${RENDER_SOURCES})
