#include "obj_loader.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <fstream>

namespace {

bool load_obj_mt(const char *path, Scene &scene, Vector3df &bounds_min, Vector3df &bounds_max) {
  return load_obj(path, scene, TriangleMethod::MoellerTrumbore, bounds_min, bounds_max);
}

const TriangleObject &triangle_of(const Scene &scene, size_t index) {
  return dynamic_cast<const TriangleObject &>(*scene.objects[index]);
}

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
  Scene scene;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj_mt("test_triangle.obj", scene, bounds_min, bounds_max) );
  ASSERT_EQ(1u, scene.objects.size());
  EXPECT_NE(nullptr, dynamic_cast<const MoellerTrumboreTriangle *>(scene.objects[0].get()));
  EXPECT_NEAR(1.0f, triangle_of(scene, 0).geometry().get_na()[2], 0.00001);
  EXPECT_NEAR(1.0f, bounds_max[0], 0.00001);
  EXPECT_NEAR(1.0f, bounds_max[1], 0.00001);
  EXPECT_NEAR(0.0f, bounds_min[2], 0.00001);
  std::remove("test_triangle.obj");
}

TEST(OBJ_LOADER, SplitsQuadIntoTwoTriangles) {
  write_file("test_quad.obj", "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nf 1 2 3 4\n");
  Scene scene;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj_mt("test_quad.obj", scene, bounds_min, bounds_max) );
  EXPECT_EQ(2u, scene.objects.size());
  std::remove("test_quad.obj");
}

TEST(OBJ_LOADER, SupportsNegativeIndices) {
  write_file("test_negative.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
  Scene scene;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj_mt("test_negative.obj", scene, bounds_min, bounds_max) );
  ASSERT_EQ(1u, scene.objects.size());
  EXPECT_NEAR(1.0f, triangle_of(scene, 0).geometry().get_b()[0], 0.00001);
  std::remove("test_negative.obj");
}

TEST(OBJ_LOADER, ReadsMaterialsFromMtlFile) {
  write_file("test_materials.mtl",
             "newmtl red\nKa 0.2 0 0\nKd 1 0 0   # comment\n\n"
             "newmtl porcelain\nKd 0.8 0.8 0.8\nKs 0.4 0.4 0.4\nNs 5\nillum 2\n");
  write_file("test_materials.obj",
             "mtllib test_materials.mtl\n"
             "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
             "usemtl red # first\nf 1 2 3\n"
             "usemtl porcelain\nf 1 2 3\n");
  Scene scene;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj_mt("test_materials.obj", scene, bounds_min, bounds_max) );
  ASSERT_EQ(2u, scene.objects.size());
  const Material &red = scene.objects[0]->material;
  EXPECT_NEAR(0.2f, red.ambient[0], 0.00001);
  EXPECT_NEAR(1.0f, red.diffuse[0], 0.00001);
  EXPECT_NEAR(0.0f, red.diffuse[1], 0.00001);
  EXPECT_FALSE( red.reflects() );

  // Ks allein macht das Material spiegelnd, illum spielt keine Rolle
  const Material &porcelain = scene.objects[1]->material;
  EXPECT_TRUE( porcelain.reflects() );
  EXPECT_NEAR(0.4f, porcelain.specular[0], 0.00001);
  EXPECT_NEAR(5.0f, porcelain.shininess, 0.00001);
  std::remove("test_materials.obj");
  std::remove("test_materials.mtl");
}

// d < 1 macht das Material durchsichtig mit dem Anteil 1 - d
TEST(OBJ_LOADER, TransparentMaterialFromDissolve) {
  write_file("test_glass.mtl", "newmtl glass\nKd 0.3 0.3 0.3\nKs 0.3 0.3 0.3\nd 0.3   # durchsichtig\nNi 1.33\n");
  write_file("test_glass.obj", "mtllib test_glass.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl glass\nf 1 2 3\n");
  Scene scene;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj_mt("test_glass.obj", scene, bounds_min, bounds_max) );
  ASSERT_EQ(1u, scene.objects.size());
  const Material &glass = scene.objects[0]->material;
  EXPECT_TRUE( glass.transmits() );
  EXPECT_NEAR(0.7f, glass.transparency, 0.00001);
  EXPECT_NEAR(1.33f, glass.refraction_index, 0.00001);
  EXPECT_NEAR(0.3f, glass.diffuse[0], 0.00001);
  std::remove("test_glass.obj");
  std::remove("test_glass.mtl");
}

// das Schnittverfahren wird beim Einlesen festgelegt
TEST(OBJ_LOADER, CreatesTrianglesForChosenMethod) {
  write_file("test_method.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
  Scene scene;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};

  ASSERT_TRUE( load_obj("test_method.obj", scene, TriangleMethod::Badouel, bounds_min, bounds_max) );
  EXPECT_NE(nullptr, dynamic_cast<const BadouelTriangle *>(scene.objects[0].get()));
  std::remove("test_method.obj");
}

TEST(OBJ_LOADER, FailsForMissingFileOrEmptyScene) {
  Scene scene;
  Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};
  EXPECT_FALSE( load_obj_mt("does_not_exist.obj", scene, bounds_min, bounds_max) );

  write_file("test_empty.obj", "v 0 0 0\n");
  EXPECT_FALSE( load_obj_mt("test_empty.obj", scene, bounds_min, bounds_max) );
  std::remove("test_empty.obj");
}

}
