#include "camera.h"
#include "shader.h"
#include "gtest/gtest.h"

namespace {

// ---------------------------------------------------------------- Kamera

TEST(CAMERA, CenterRayLooksAtTarget) {
  Camera camera(101, 51);
  camera.place({1.f, 2.f, 3.f}, {1.f, 2.f, -7.f}, 60.f);
  Ray3df ray = camera.primary_ray(50.f, 25.f);   // mittleres Pixel
  Vector3df direction = ray.direction;
  direction.normalize();
  EXPECT_NEAR(1.0f, ray.origin[0], 0.0001);
  EXPECT_NEAR(0.0f, direction[0], 0.0001);
  EXPECT_NEAR(0.0f, direction[1], 0.0001);
  EXPECT_NEAR(-1.0f, direction[2], 0.0001);
}

TEST(CAMERA, UpperLeftPixelLiesLeftAndUp) {
  Camera camera(100, 100);
  Ray3df ray = camera.primary_ray(0.f, 0.f);
  EXPECT_LT(ray.direction[0], 0.f);
  EXPECT_GT(ray.direction[1], 0.f);
}

// ---------------------------------------------------------------- direktes Licht

// Boden in der Ebene y = 0 und eine Lichtquelle direkt darueber
class DirectLightTest : public ::testing::Test {
protected:
  Scene scene;
  Shader shader{scene};
  Material material = Material::matte({1.f, 1.f, 1.f});   // Ka = 0.1, Kd = 0.9
  Vector3df point = {0.f, 0.f, 0.f};
  Vector3df up = {0.f, 1.f, 0.f};

  void SetUp() override {
    scene.lights.push_back(PointLight({0.f, 10.f, 0.f}));
  }
};

TEST_F(DirectLightTest, LitPointFacingLightIsFullyBright) {
  EXPECT_NEAR(1.0f, shader.direct_light(material, point, up, up)[0], 0.0001);
}

TEST_F(DirectLightTest, ShadowedPointGetsAmbientOnly) {
  scene.add(std::make_unique<SphereObject>(Sphere3df({0.f, 5.f, 0.f}, 1.f), Material()));
  EXPECT_NEAR(0.1f, shader.direct_light(material, point, up, up)[0], 0.0001);
}

TEST_F(DirectLightTest, PointFacingAwayGetsAmbientWithoutShadowRay) {
  EXPECT_NEAR(0.1f, shader.direct_light(material, point, -1.f * up, up)[0], 0.0001);
  EXPECT_EQ(0u, scene.any_queries);   // es wurde kein Schattenstrahl verfolgt
}

TEST_F(DirectLightTest, ObjectBehindLightCastsNoShadow) {
  scene.add(std::make_unique<SphereObject>(Sphere3df({0.f, 20.f, 0.f}, 1.f), Material()));
  EXPECT_NEAR(1.0f, shader.direct_light(material, point, up, up)[0], 0.0001);
}

// Ks und Ns aus der Materialdatei ergeben ein Glanzlicht, das nur in Spiegelrichtung des Lichts hell ist
TEST_F(DirectLightTest, SpecularHighlightPeaksInMirrorDirection) {
  Material glossy;
  glossy.ambient = {0.f, 0.f, 0.f};
  glossy.diffuse = {0.f, 0.f, 0.f};
  glossy.specular = {0.5f, 0.5f, 0.5f};
  glossy.shininess = 50.f;

  EXPECT_NEAR(0.5f, shader.direct_light(glossy, point, up, up)[0], 0.0001);   // Auge in Spiegelrichtung
  Vector3df oblique = {1.f, 1.f, 0.f};
  oblique.normalize();
  EXPECT_LT(shader.direct_light(glossy, point, up, oblique)[0], 0.001f);      // 45 Grad daneben
}

// ---------------------------------------------------------------- Rekursion

TEST(SHADER, RecursionDepthZeroIsBlack) {
  Scene scene;
  Shader shader(scene);
  color c = shader.trace(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, 0);
  EXPECT_EQ(0.f, c[0]);
  EXPECT_EQ(0.f, c[1]);
  EXPECT_EQ(0.f, c[2]);
}

// Ks ist das Gewicht der Spiegelung: ein halb spiegelndes schwarzes Dreieck zeigt den halben roten Hintergrund
TEST(SHADER, ReflectionIsWeightedWithKs) {
  Scene scene;
  scene.lights.push_back(PointLight({0.f, 0.f, 10.f}));
  Material half_mirror;
  half_mirror.ambient = {0.f, 0.f, 0.f};
  half_mirror.diffuse = {0.f, 0.f, 0.f};
  half_mirror.specular = {0.5f, 0.5f, 0.5f};
  half_mirror.shininess = 1000.f;
  Material red = Material::matte({1.f, 0.f, 0.f});
  scene.add(make_triangle(TriangleMethod::MoellerTrumbore, Triangle3df({-50.f, -50.f, -5.f}, {50.f, -50.f, -5.f}, {0.f, 50.f, -5.f}), half_mirror));
  scene.add(make_triangle(TriangleMethod::MoellerTrumbore, Triangle3df({-50.f, -50.f, 5.f}, {0.f, 50.f, 5.f}, {50.f, -50.f, 5.f}), red));

  // Licht hinter der roten Flaeche, das Auge sieht die rote Flaeche also nur ambient (0.1) und gespiegelt
  Shader shader(scene);
  color c = shader.trace(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, 5);
  EXPECT_NEAR(0.05f, c[0], 0.001);
  EXPECT_NEAR(0.0f, c[1], 0.001);
}

// Die eigene Farbe einer durchsichtigen Flaeche zaehlt nur mit der Deckkraft d, der Rest kommt von dahinter
TEST(SHADER, OwnColorOfTransparentSurfaceIsWeightedWithOpacity) {
  Scene scene;
  scene.lights.push_back(PointLight({0.f, 0.f, 10.f}));
  Material veil;   // weiss und zu 70 % durchsichtig, ohne Brechung (Ni = 1)
  veil.ambient = {0.f, 0.f, 0.f};
  veil.diffuse = {1.f, 1.f, 1.f};
  veil.transparency = 0.7f;
  veil.refraction_index = 1.f;
  scene.add(make_triangle(TriangleMethod::MoellerTrumbore, Triangle3df({-50.f, -50.f, -5.f}, {50.f, -50.f, -5.f}, {0.f, 50.f, -5.f}), veil));

  // hinter der Flaeche nur Himmel, in Blickrichtung -z ist er (0.75, 0.85, 1)
  Shader shader(scene);
  color c = shader.trace(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, 5);
  EXPECT_NEAR(0.3f * 1.f + 0.7f * 0.75f, c[0], 0.001);
  EXPECT_NEAR(0.3f * 1.f + 0.7f * 1.f, c[2], 0.001);
}

// Ein Strahl durch die Mitte einer Glaskugel wird nicht abgelenkt und trifft die rote Wand dahinter.
// Das Licht steht zwischen Kugel und Wand, da Glas einen vollen Schatten wirft.
TEST(SHADER, RayThroughGlassSphereSeesObjectBehind) {
  Scene scene;
  scene.lights.push_back(PointLight({0.f, 0.f, -10.f}));
  Material glass;
  glass.ambient = {0.f, 0.f, 0.f};
  glass.diffuse = {0.f, 0.f, 0.f};
  glass.transparency = 1.f;
  scene.add(std::make_unique<SphereObject>(Sphere3df({0.f, 0.f, -5.f}, 1.f), glass));
  scene.add(make_triangle(TriangleMethod::MoellerTrumbore, Triangle3df({-50.f, -50.f, -20.f}, {50.f, -50.f, -20.f}, {0.f, 50.f, -20.f}), Material::matte({1.f, 0.f, 0.f})));

  Shader shader(scene);
  color c = shader.trace(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, 5);
  EXPECT_GT(c[0], 0.9f);   // die rote Wand, voll beleuchtet
  EXPECT_LT(c[1], 0.01f);
  EXPECT_LT(c[2], 0.01f);
}

}
