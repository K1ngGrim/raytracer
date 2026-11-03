#include "obj_loader.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <fstream>

namespace {

// schreibt eine kleine Wavefront-Datei in das Arbeitsverzeichnis
void write_file(const char *name, const char *content) {
  std::ofstream file(name);
  file << content;
}

TEST(OBJ_LOADER, ReadsTriangleWithVertexNormals) {
  write_file("test_triangle.obj",
             "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
             "vn 0 0 1\n"
             "f 1//1 2//1 3//1\n");
  World world;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj("test_triangle.obj", world, bounds_min, bounds_max) );
  ASSERT_EQ(1u, world.objects.size());
  EXPECT_TRUE( world.objects[0].is_triangle );
  EXPECT_NEAR(1.0f, world.objects[0].triangle.get_na()[2], 0.00001);
  EXPECT_NEAR(1.0f, bounds_max[0], 0.00001);
  EXPECT_NEAR(1.0f, bounds_max[1], 0.00001);
  EXPECT_NEAR(0.0f, bounds_min[2], 0.00001);
  std::remove("test_triangle.obj");
}

TEST(OBJ_LOADER, SplitsQuadIntoTwoTriangles) {
  write_file("test_quad.obj", "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");
  World world;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj("test_quad.obj", world, bounds_min, bounds_max) );
  EXPECT_EQ(2u, world.objects.size());
  std::remove("test_quad.obj");
}

TEST(OBJ_LOADER, SupportsNegativeIndices) {
  write_file("test_negative.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
  World world;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj("test_negative.obj", world, bounds_min, bounds_max) );
  ASSERT_EQ(1u, world.objects.size());
  EXPECT_NEAR(1.0f, world.objects[0].triangle.get_b()[0], 0.00001);
  std::remove("test_negative.obj");
}

TEST(OBJ_LOADER, ReadsMaterialsFromMtlFile) {
  write_file("test_materials.mtl", "newmtl red\nKd 1 0 0   # comment\n\nnewmtl mirror\nKs 0.5 0.5 0.5\nillum 3\n");
  write_file("test_materials.obj",
             "mtllib test_materials.mtl\n"
             "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
             "usemtl red # first\nf 1 2 3\n"
             "usemtl mirror\nf 1 2 3\n");
  World world;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj("test_materials.obj", world, bounds_min, bounds_max) );
  ASSERT_EQ(2u, world.objects.size());
  EXPECT_EQ(0, world.objects[0].material.type);
  EXPECT_NEAR(1.0f, world.objects[0].material.materialColor[0], 0.00001);
  EXPECT_NEAR(0.0f, world.objects[0].material.materialColor[1], 0.00001);
  EXPECT_EQ(1, world.objects[1].material.type);
  EXPECT_NEAR(0.5f, world.objects[1].material.materialColor[0], 0.00001);
  std::remove("test_materials.obj");
  std::remove("test_materials.mtl");
}

// d < 1 macht das Material durchsichtig, die Farbe Kd spielt dann keine Rolle
TEST(OBJ_LOADER, TransparentMaterialBecomesGlass) {
  write_file("test_glass.mtl", "newmtl glass\nKd 0.3 0.3 0.3\nd 0.3   # durchsichtig\nNi 1.33\n");
  write_file("test_glass.obj", "mtllib test_glass.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl glass\nf 1 2 3\n");
  World world;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj("test_glass.obj", world, bounds_min, bounds_max) );
  ASSERT_EQ(1u, world.objects.size());
  EXPECT_EQ(GLASS, world.objects[0].material.type);
  EXPECT_NEAR(1.33f, world.objects[0].material.refractionIndex, 0.00001);
  EXPECT_NEAR(1.0f, world.objects[0].material.materialColor[0], 0.00001);
  std::remove("test_glass.obj");
  std::remove("test_glass.mtl");
}

TEST(OBJ_LOADER, FailsForMissingFileOrEmptyScene) {
  World world;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};
  EXPECT_FALSE( load_obj("does_not_exist.obj", world, bounds_min, bounds_max) );

  write_file("test_empty.obj", "v 0 0 0\n");
  EXPECT_FALSE( load_obj("test_empty.obj", world, bounds_min, bounds_max) );
  std::remove("test_empty.obj");
}

// Dreiecke werden mit der Normale gegen den Strahl beleuchtet, unabhaengig von der Orientierung
TEST(WORLD, FindsNearestTriangleAndFacesNormalToRay) {
  World world;
  Triangle3df triangle({-1.f, -1.f, 0.f}, {1.f, -1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f});
  world.add(WorldObject(triangle, Material({1.f, 0.f, 0.f}, 0)));

  Intersection_Context<float, 3> context{};
  WorldObject *hit = world.find_nearest_object(Ray3df{{0.f, 0.f, 5.f}, {0.f, 0.f, -1.f}}, context);   // von vorne
  ASSERT_NE(nullptr, hit);
  EXPECT_NEAR(5.0f, context.t, 0.0001);
  EXPECT_NEAR(1.0f, context.normal[2], 0.0001);

  hit = world.find_nearest_object(Ray3df{{0.f, 0.f, -5.f}, {0.f, 0.f, 1.f}}, context);   // von hinten
  ASSERT_NE(nullptr, hit);
  EXPECT_NEAR(-1.0f, context.normal[2], 0.0001);

  EXPECT_EQ(nullptr, world.find_nearest_object(Ray3df{{5.f, 5.f, 5.f}, {0.f, 0.f, -1.f}}, context));   // daneben
}

}
