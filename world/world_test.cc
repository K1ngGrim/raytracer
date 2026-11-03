#include "world.h"
#include "../view/camera.h"
#include "gtest/gtest.h"
#include <cstdlib>

namespace {

// Dreieck in der Ebene z = z0, das den Bereich um die z-Achse abdeckt
Triangle3df triangle_at(float z0) {
  return Triangle3df({-1.f, -1.f, z0}, {1.f, -1.f, z0}, {0.f, 1.f, z0}, {0.f, 0.f, 1.f});
}

float random_in(float min, float max) {
  return min + (max - min) * float(rand()) / float(RAND_MAX);
}

// ---------------------------------------------------------------- Schattentest

TEST(WORLD, RayIntersectsAnyFindsBlockerBeforeLight) {
  World world;
  world.add(WorldObject(triangle_at(0.f), Material({1.f, 1.f, 1.f}, LAMBERTIAN)));

  // das Licht liegt bei t = 1, das Dreieck bei t = 0.5
  EXPECT_TRUE( world.ray_intersects_any(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -2.f}}, 1.f) );
}

TEST(WORLD, RayIntersectsAnyIgnoresObjectBehindLight) {
  World world;
  world.add(WorldObject(triangle_at(0.f), Material({1.f, 1.f, 1.f}, LAMBERTIAN)));

  // das Dreieck liegt bei t = 2, also hinter dem Licht
  EXPECT_FALSE( world.ray_intersects_any(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -0.5f}}, 1.f) );
  // und gar nicht in Richtung des Strahls
  EXPECT_FALSE( world.ray_intersects_any(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}}, 1.f) );
}

TEST(WORLD, RayIntersectsAnyStopsAtFirstHit) {
  World world;
  world.add(WorldObject(triangle_at(0.f), Material({1.f, 1.f, 1.f}, LAMBERTIAN)));
  for (int i = 0; i < 9; ++i)   // neun Dreiecke abseits des Strahls
    world.add(WorldObject(Triangle3df({10.f, 10.f, 0.f}, {11.f, 10.f, 0.f}, {10.f, 11.f, 0.f}), Material({1.f, 1.f, 1.f}, LAMBERTIAN)));

  ASSERT_TRUE( world.ray_intersects_any(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -2.f}}, 1.f) );
  EXPECT_EQ(1u, world.intersection_tests);   // nur das erste Dreieck wurde getestet

  Intersection_Context<float, 3> context{};
  world.find_nearest_object(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -1.f}}, context);
  EXPECT_EQ(11u, world.intersection_tests);  // die Suche nach dem naechsten Objekt testet alle zehn
}

// ---------------------------------------------------------------- Brute-Force-Suche

TEST(WORLD, FindsNearestOfSeveralObjects) {
  World world;
  world.add(WorldObject(triangle_at(-5.f), Material({1.f, 0.f, 0.f}, LAMBERTIAN)));
  world.add(WorldObject(triangle_at(-2.f), Material({0.f, 1.f, 0.f}, LAMBERTIAN)));
  world.add(WorldObject(Sphere3df({0.f, 0.f, -10.f}, 1.f), Material({0.f, 0.f, 1.f}, LAMBERTIAN)));

  Intersection_Context<float, 3> context{};
  WorldObject *hit = world.find_nearest_object(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, context);
  ASSERT_NE(nullptr, hit);
  EXPECT_EQ(&world.objects[1], hit);
  EXPECT_NEAR(2.0f, context.t, 0.0001);
}

// Beide Schnittverfahren muessen in derselben Szene dasselbe Objekt im selben Abstand finden
TEST(WORLD, MoellerTrumboreAndBadouelFindSameNearestObject) {
  srand(42);
  World world;
  for (int i = 0; i < 200; ++i) {
    Vector3df a = {random_in(-5.f, 5.f), random_in(-5.f, 5.f), random_in(-20.f, -5.f)};
    Vector3df b = a + Vector3df{random_in(-2.f, 2.f), random_in(-2.f, 2.f), random_in(-2.f, 2.f)};
    Vector3df c = a + Vector3df{random_in(-2.f, 2.f), random_in(-2.f, 2.f), random_in(-2.f, 2.f)};
    world.add(WorldObject(Triangle3df(a, b, c), Material({1.f, 1.f, 1.f}, LAMBERTIAN)));
  }

  int hits = 0;
  for (int i = 0; i < 2000; ++i) {
    Ray3df ray = {{0.f, 0.f, 0.f}, {random_in(-0.4f, 0.4f), random_in(-0.4f, 0.4f), -1.f}};
    Intersection_Context<float, 3> mt{}, badouel{};

    world.moeller_trumbore = true;
    WorldObject *hit_mt = world.find_nearest_object(ray, mt);
    world.moeller_trumbore = false;
    WorldObject *hit_badouel = world.find_nearest_object(ray, badouel);

    ASSERT_EQ(hit_mt, hit_badouel) << "Strahl " << i;
    if (hit_mt != nullptr) {
      ++hits;
      EXPECT_NEAR(mt.t, badouel.t, 1e-3f * mt.t);
      EXPECT_NEAR(mt.u, badouel.u, 1e-3f);
      EXPECT_NEAR(mt.v, badouel.v, 1e-3f);
    }
  }
  EXPECT_GT(hits, 100);   // sonst waere der Test wenig aussagekraeftig
}

// ---------------------------------------------------------------- Innen und aussen

TEST(WORLD_OBJECT, RayStartsInsideTriangleDependsOnOrientation) {
  // (b - a) x (c - a) zeigt in +z, das ist die Aussenseite
  WorldObject obj(Triangle3df({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}), Material({1.f, 1.f, 1.f}, GLASS));
  EXPECT_FALSE( obj.ray_starts_inside(Ray3df{{0.2f, 0.2f, 1.f}, {0.f, 0.f, -1.f}}) );   // tritt ein
  EXPECT_TRUE( obj.ray_starts_inside(Ray3df{{0.2f, 0.2f, -1.f}, {0.f, 0.f, 1.f}}) );    // tritt aus
}

TEST(WORLD_OBJECT, RayStartsInsideSphere) {
  WorldObject obj(Sphere3df({0.f, 0.f, 0.f}, 1.f), Material({1.f, 1.f, 1.f}, GLASS));
  EXPECT_TRUE( obj.ray_starts_inside(Ray3df{{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}}) );
  EXPECT_FALSE( obj.ray_starts_inside(Ray3df{{0.f, 0.f, 5.f}, {0.f, 0.f, -1.f}}) );
}

// ---------------------------------------------------------------- Material

TEST(MATERIAL, InvalidTypeFallsBackToLambertian) {
  EXPECT_EQ(LAMBERTIAN, Material({1.f, 1.f, 1.f}, 7).type);
  EXPECT_EQ(LAMBERTIAN, Material({1.f, 1.f, 1.f}, -1).type);
  EXPECT_EQ(GLASS, Material({1.f, 1.f, 1.f}, GLASS, 1.33f).type);
  EXPECT_NEAR(1.33f, Material({1.f, 1.f, 1.f}, GLASS, 1.33f).refractionIndex, 0.00001);
}

// ---------------------------------------------------------------- Beleuchtung

// Boden in der Ebene y = 0 und eine Lichtquelle direkt darueber
class LambertianTest : public ::testing::Test {
protected:
  World world;
  Intersection_Context<float, 3> context{};

  void SetUp() override {
    world.lights.push_back(PointLight({0.f, 10.f, 0.f}));
    context.intersection = {0.f, 0.f, 0.f};
    context.normal = {0.f, 1.f, 0.f};
  }
};

TEST_F(LambertianTest, LitPointFacingLightIsFullyBright) {
  EXPECT_NEAR(1.0f, Camera::lambertian(&world, world.lights[0], context), 0.0001);
}

TEST_F(LambertianTest, ShadowedPointGetsAmbientOnly) {
  world.add(WorldObject(Sphere3df({0.f, 5.f, 0.f}, 1.f), Material({1.f, 1.f, 1.f}, LAMBERTIAN)));
  EXPECT_NEAR(0.1f, Camera::lambertian(&world, world.lights[0], context), 0.0001);
}

TEST_F(LambertianTest, PointFacingAwayGetsAmbientWithoutShadowRay) {
  context.normal = {0.f, -1.f, 0.f};
  EXPECT_NEAR(0.1f, Camera::lambertian(&world, world.lights[0], context), 0.0001);
  EXPECT_EQ(0u, world.any_queries);   // es wurde kein Schattenstrahl verfolgt
}

TEST_F(LambertianTest, ObjectBehindLightCastsNoShadow) {
  world.add(WorldObject(Sphere3df({0.f, 20.f, 0.f}, 1.f), Material({1.f, 1.f, 1.f}, LAMBERTIAN)));
  EXPECT_NEAR(1.0f, Camera::lambertian(&world, world.lights[0], context), 0.0001);
}

// ---------------------------------------------------------------- Glas

// Ein Strahl durch die Mitte einer Glaskugel wird nicht abgelenkt und trifft die rote Wand dahinter.
// Das Licht steht zwischen Kugel und Wand, da Glas einen vollen Schatten wirft.
TEST(CAMERA, RayThroughGlassSphereSeesObjectBehind) {
  World world;
  world.lights.push_back(PointLight({0.f, 0.f, -10.f}));
  world.add(WorldObject(Sphere3df({0.f, 0.f, -5.f}, 1.f), Material({1.f, 1.f, 1.f}, GLASS, 1.5f)));
  world.add(WorldObject(Triangle3df({-50.f, -50.f, -20.f}, {50.f, -50.f, -20.f}, {0.f, 50.f, -20.f}), Material({1.f, 0.f, 0.f}, LAMBERTIAN)));

  Camera camera(100.f, 100.f);
  color c = camera.ray_color(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, &world, 5);
  EXPECT_GT(c[0], 0.5f);   // ueberwiegend die rote Wand
  EXPECT_LT(c[1], 0.2f);   // nur der Fresnel-Anteil der Spiegelung (Himmel) bringt Gruen und Blau
  EXPECT_LT(c[2], 0.2f);
}

TEST(CAMERA, RecursionDepthZeroIsBlack) {
  World world;
  world.lights.push_back(PointLight({0.f, 0.f, 10.f}));
  Camera camera(100.f, 100.f);
  color c = camera.ray_color(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, &world, 0);
  EXPECT_EQ(0.f, c[0]);
  EXPECT_EQ(0.f, c[1]);
  EXPECT_EQ(0.f, c[2]);
}

}
