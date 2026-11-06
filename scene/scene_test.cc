#include "scene.h"
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

std::unique_ptr<TriangleObject> triangle(Triangle3df geometry, TriangleMethod method = TriangleMethod::MoellerTrumbore) {
  return make_triangle(method, geometry, Material::matte({1.f, 1.f, 1.f}));
}

// ---------------------------------------------------------------- Schattentest

TEST(SCENE, AnyHitFindsBlockerBeforeLight) {
  Scene scene;
  scene.add(triangle(triangle_at(0.f)));

  // das Licht liegt bei t = 1, das Dreieck bei t = 0.5
  EXPECT_TRUE( scene.any_hit(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -2.f}}, 1.f) );
}

TEST(SCENE, AnyHitIgnoresObjectBehindLight) {
  Scene scene;
  scene.add(triangle(triangle_at(0.f)));

  // das Dreieck liegt bei t = 2, also hinter dem Licht
  EXPECT_FALSE( scene.any_hit(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -0.5f}}, 1.f) );
  // und gar nicht in Richtung des Strahls
  EXPECT_FALSE( scene.any_hit(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}}, 1.f) );
}

TEST(SCENE, AnyHitStopsAtFirstHit) {
  Scene scene;
  scene.add(triangle(triangle_at(0.f)));
  for (int i = 0; i < 9; ++i)   // neun Dreiecke abseits des Strahls
    scene.add(triangle(Triangle3df({10.f, 10.f, 0.f}, {11.f, 10.f, 0.f}, {10.f, 11.f, 0.f})));

  ASSERT_TRUE( scene.any_hit(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -2.f}}, 1.f) );
  EXPECT_EQ(1u, scene.intersection_tests);   // nur das erste Dreieck wurde getestet

  Intersection_Context<float, 3> context{};
  scene.nearest_hit(Ray3df{{0.f, 0.f, 1.f}, {0.f, 0.f, -1.f}}, context);
  EXPECT_EQ(11u, scene.intersection_tests);  // die Suche nach dem naechsten Treffer testet alle zehn
}

// ---------------------------------------------------------------- Brute-Force-Suche

TEST(SCENE, FindsNearestOfSeveralObjects) {
  Scene scene;
  scene.add(triangle(triangle_at(-5.f)));
  scene.add(triangle(triangle_at(-2.f), TriangleMethod::Badouel));
  scene.add(std::make_unique<SphereObject>(Sphere3df({0.f, 0.f, -10.f}, 1.f), Material::matte({0.f, 0.f, 1.f})));

  Intersection_Context<float, 3> context{};
  const SceneObject *hit = scene.nearest_hit(Ray3df{{0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}}, context);
  ASSERT_NE(nullptr, hit);
  EXPECT_EQ(scene.objects[1].get(), hit);
  EXPECT_NEAR(2.0f, context.t, 0.0001);
}

// Dreiecke werden mit der Normale gegen den Strahl beleuchtet, unabhaengig von der Orientierung
TEST(SCENE, NearestHitFacesNormalToRay) {
  Scene scene;
  scene.add(triangle(triangle_at(0.f)));

  Intersection_Context<float, 3> context{};
  ASSERT_NE(nullptr, scene.nearest_hit(Ray3df{{0.f, 0.f, 5.f}, {0.f, 0.f, -1.f}}, context));   // von vorne
  EXPECT_NEAR(5.0f, context.t, 0.0001);
  EXPECT_NEAR(1.0f, context.normal[2], 0.0001);

  ASSERT_NE(nullptr, scene.nearest_hit(Ray3df{{0.f, 0.f, -5.f}, {0.f, 0.f, 1.f}}, context));   // von hinten
  EXPECT_NEAR(-1.0f, context.normal[2], 0.0001);

  EXPECT_EQ(nullptr, scene.nearest_hit(Ray3df{{5.f, 5.f, 5.f}, {0.f, 0.f, -1.f}}, context));   // daneben
}

// ---------------------------------------------------------------- Schnittverfahren

TEST(BADOUEL, HitsTriangleInEveryOrientation) {
  // die Normale zeigt einmal in jede Achsrichtung, damit jede Projektion einmal vorkommt
  Triangle3df xy({0.f, 0.f, 0.f}, {2.f, 0.f, 0.f}, {0.f, 2.f, 0.f});
  Triangle3df yz({0.f, 0.f, 0.f}, {0.f, 2.f, 0.f}, {0.f, 0.f, 2.f});
  Triangle3df xz({0.f, 0.f, 0.f}, {0.f, 0.f, 2.f}, {2.f, 0.f, 0.f});
  Intersection_Context<float, 3> context{};

  ASSERT_TRUE( BadouelTriangle(xy, Material()).intersects(Ray3df{{0.5f, 0.5f, 3.f}, {0.f, 0.f, -1.f}}, context) );
  EXPECT_NEAR(3.0f, context.t, 0.0001);
  EXPECT_NEAR(0.25f, context.v, 0.0001);   // Gewicht von b
  ASSERT_TRUE( BadouelTriangle(yz, Material()).intersects(Ray3df{{3.f, 0.5f, 0.5f}, {-1.f, 0.f, 0.f}}, context) );
  EXPECT_NEAR(3.0f, context.t, 0.0001);
  ASSERT_TRUE( BadouelTriangle(xz, Material()).intersects(Ray3df{{0.5f, 3.f, 0.5f}, {0.f, -1.f, 0.f}}, context) );
  EXPECT_NEAR(3.0f, context.t, 0.0001);
}

TEST(BADOUEL, MissesBesideBehindAndParallel) {
  BadouelTriangle object(Triangle3df({0.f, 0.f, 0.f}, {0.f, 3.f, 0.f}, {3.f, 0.f, 0.f}), Material());
  Intersection_Context<float, 3> context{};

  EXPECT_FALSE( object.intersects(Ray3df{{2.f, 2.f, 2.f}, {0.f, 0.f, -1.f}}, context) );   // neben der Hypotenuse
  EXPECT_FALSE( object.intersects(Ray3df{{1.f, 1.f, 2.f}, {0.f, 0.f, 1.f}}, context) );    // Dreieck hinter dem Strahl
  EXPECT_FALSE( object.intersects(Ray3df{{1.f, 1.f, 2.f}, {1.f, 0.f, 0.f}}, context) );    // parallel
}

// b - a steht senkrecht auf der ersten Projektionsachse, dafuer hat Badouel einen eigenen Zweig
TEST(BADOUEL, HandlesEdgeParallelToProjectionAxis) {
  BadouelTriangle object(Triangle3df({0.f, 0.f, 0.f}, {0.f, 2.f, 0.f}, {2.f, 0.f, 0.f}), Material());
  Intersection_Context<float, 3> context{};

  ASSERT_TRUE( object.intersects(Ray3df{{0.5f, 0.5f, 1.f}, {0.f, 0.f, -1.f}}, context) );
  EXPECT_NEAR(0.25f, context.v, 0.0001);         // Gewicht von b
  EXPECT_NEAR(0.5f, context.u, 0.0001);          // Gewicht von a
  EXPECT_FALSE( object.intersects(Ray3df{{1.5f, 1.5f, 1.f}, {0.f, 0.f, -1.f}}, context) );
}

// Beide Schnittverfahren muessen in derselben Szene dasselbe Objekt im selben Abstand finden
TEST(SCENE, MoellerTrumboreAndBadouelFindSameNearestObject) {
  srand(42);
  Scene moeller, badouel;
  for (int i = 0; i < 200; ++i) {
    Vector3df a = {random_in(-5.f, 5.f), random_in(-5.f, 5.f), random_in(-20.f, -5.f)};
    Vector3df b = a + Vector3df{random_in(-2.f, 2.f), random_in(-2.f, 2.f), random_in(-2.f, 2.f)};
    Vector3df c = a + Vector3df{random_in(-2.f, 2.f), random_in(-2.f, 2.f), random_in(-2.f, 2.f)};
    moeller.add(triangle(Triangle3df(a, b, c), TriangleMethod::MoellerTrumbore));
    badouel.add(triangle(Triangle3df(a, b, c), TriangleMethod::Badouel));
  }

  int hits = 0;
  for (int i = 0; i < 2000; ++i) {
    Ray3df ray = {{0.f, 0.f, 0.f}, {random_in(-0.4f, 0.4f), random_in(-0.4f, 0.4f), -1.f}};
    Intersection_Context<float, 3> mt{}, bd{};
    const SceneObject *hit_mt = moeller.nearest_hit(ray, mt);
    const SceneObject *hit_bd = badouel.nearest_hit(ray, bd);

    ASSERT_EQ(hit_mt == nullptr, hit_bd == nullptr) << "Strahl " << i;
    if (hit_mt != nullptr) {
      ++hits;
      // dasselbe Dreieck an derselben Stelle der Liste
      size_t index_mt = 0, index_bd = 0;
      for (size_t k = 0; k < moeller.objects.size(); ++k) {
        if (moeller.objects[k].get() == hit_mt) index_mt = k;
        if (badouel.objects[k].get() == hit_bd) index_bd = k;
      }
      EXPECT_EQ(index_mt, index_bd) << "Strahl " << i;
      EXPECT_NEAR(mt.t, bd.t, 1e-3f * mt.t);
      EXPECT_NEAR(mt.u, bd.u, 1e-3f);
      EXPECT_NEAR(mt.v, bd.v, 1e-3f);
    }
  }
  EXPECT_GT(hits, 100);   // sonst waere der Test wenig aussagekraeftig
}

// ---------------------------------------------------------------- Innen und aussen

TEST(SCENE_OBJECT, RayStartsInsideTriangleDependsOnOrientation) {
  // (b - a) x (c - a) zeigt in +z, das ist die Aussenseite
  auto object = triangle(Triangle3df({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}));
  EXPECT_FALSE( object->ray_starts_inside(Ray3df{{0.2f, 0.2f, 1.f}, {0.f, 0.f, -1.f}}) );   // tritt ein
  EXPECT_TRUE( object->ray_starts_inside(Ray3df{{0.2f, 0.2f, -1.f}, {0.f, 0.f, 1.f}}) );    // tritt aus
}

TEST(SCENE_OBJECT, RayStartsInsideSphere) {
  SphereObject object(Sphere3df({0.f, 0.f, 0.f}, 1.f), Material());
  EXPECT_TRUE( object.ray_starts_inside(Ray3df{{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}}) );
  EXPECT_FALSE( object.ray_starts_inside(Ray3df{{0.f, 0.f, 5.f}, {0.f, 0.f, -1.f}}) );
}

// ---------------------------------------------------------------- Material

TEST(MATERIAL, ReflectsAndTransmitsFollowKsAndD) {
  Material material;
  EXPECT_FALSE( material.reflects() );
  EXPECT_FALSE( material.transmits() );
  material.specular = {0.f, 0.3f, 0.f};
  material.transparency = 0.7f;
  EXPECT_TRUE( material.reflects() );
  EXPECT_TRUE( material.transmits() );
  EXPECT_TRUE( Material::mirror({1.f, 1.f, 1.f}).reflects() );
  EXPECT_FALSE( Material::matte({1.f, 1.f, 1.f}).reflects() );
}

}
