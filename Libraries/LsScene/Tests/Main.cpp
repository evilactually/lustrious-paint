#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <LsScene.h>

static bool Near(glm::vec3 a, glm::vec3 b, float epsilon = 1.0e-4f) {
  return glm::length(a - b) < epsilon;
}

static bool Near(const glm::mat4& a, const glm::mat4& b, float epsilon = 1.0e-4f) {
  for (int i = 0; i < 4; i++) {
    if (glm::length(a[i] - b[i]) > epsilon) {
      return false;
    }
  }
  return true;
}

TEST_CASE( "transform inverse and normals" ) {
  LsTransform t = LsTransform::Translate({1.0f, 2.0f, 3.0f})*
                  LsTransform::Rotate(0.7f, glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)))*
                  LsTransform::Scale({2.0f, 1.0f, 0.5f});
  REQUIRE(Near(t.GetMatrix()*t.GetInverseMatrix(), glm::mat4(1.0f)));

  glm::vec3 p(0.3f, -1.2f, 4.0f);
  REQUIRE(Near(t.Inverse().ApplyToPoint(t.ApplyToPoint(p)), p));

  // A transformed normal stays perpendicular to transformed tangents
  glm::vec3 n = t.ApplyToNormal({0.0f, 1.0f, 0.0f});
  REQUIRE(std::abs(glm::dot(n, t.ApplyToVector({1.0f, 0.0f, 0.0f}))) < 1.0e-4f);
  REQUIRE(std::abs(glm::dot(n, t.ApplyToVector({0.0f, 0.0f, 1.0f}))) < 1.0e-4f);
}

TEST_CASE( "transform min scale" ) {
  REQUIRE(LsTransform().GetMinScale() == Approx(1.0f));
  REQUIRE(LsTransform::Scale(glm::vec3(3.0f)).GetMinScale() == Approx(3.0f));
  LsTransform t = LsTransform::Rotate(0.4f, {0.0f, 0.0f, 1.0f})*LsTransform::Scale({2.0f, 0.5f, 4.0f});
  REQUIRE(t.GetMinScale() == Approx(0.5f));
  // Sheared: rotated non-uniform scale under another non-uniform scale
  LsTransform sheared = LsTransform::Scale({3.0f, 1.0f, 1.0f})*LsTransform::Rotate(0.8f, {0.0f, 0.0f, 1.0f});
  float minScale = sheared.GetMinScale();
  REQUIRE(minScale == Approx(1.0f));
}

TEST_CASE( "placement round trip" ) {
  LsPlacement placement;
  placement.translation = {1.0f, -2.0f, 0.5f};
  placement.rotation = {30.0f, -45.0f, 120.0f};
  placement.scale = {1.0f, 2.0f, 3.0f};
  LsPlacement decomposed = LsPlacement::FromMatrix(placement.ToTransform().GetMatrix());
  REQUIRE(Near(decomposed.translation, placement.translation));
  REQUIRE(Near(decomposed.rotation, placement.rotation, 1.0e-2f));
  REQUIRE(Near(decomposed.scale, placement.scale));

  // Gimbal lock still reproduces the same matrix
  placement.rotation = {20.0f, 90.0f, 10.0f};
  decomposed = LsPlacement::FromMatrix(placement.ToTransform().GetMatrix());
  REQUIRE(Near(decomposed.ToTransform().GetMatrix(), placement.ToTransform().GetMatrix()));

  // Mirroring
  placement.scale = {1.0f, -2.0f, 1.0f};
  decomposed = LsPlacement::FromMatrix(placement.ToTransform().GetMatrix());
  REQUIRE(Near(decomposed.ToTransform().GetMatrix(), placement.ToTransform().GetMatrix()));
}

TEST_CASE( "shape distances" ) {
  REQUIRE(LsShape::Sphere(2.0f).Distance({0.0f, 3.0f, 0.0f}) == Approx(1.0f));
  REQUIRE(LsShape::Box({1.0f, 2.0f, 3.0f}).Distance({0.0f, 0.0f, 4.0f}) == Approx(1.0f));
  REQUIRE(LsShape::Box({1.0f, 2.0f, 3.0f}).Distance({0.0f, 0.0f, 0.0f}) == Approx(-1.0f));
  REQUIRE(LsShape::Box(glm::vec3(1.0f), 0.25f).Distance({2.0f, 0.0f, 0.0f}) == Approx(1.0f));
  REQUIRE(LsShape::Capsule(1.0f, 0.5f).Distance({0.0f, 2.0f, 0.0f}) == Approx(0.5f));
  REQUIRE(LsShape::Cylinder(1.0f, 0.5f).Distance({1.0f, 0.0f, 0.0f}) == Approx(0.5f));
  REQUIRE(LsShape::Torus(1.0f, 0.25f).Distance({1.0f, 0.0f, 0.0f}) == Approx(-0.25f));
  REQUIRE(LsShape::Plane().Distance({5.0f, 2.0f, 5.0f}) == Approx(2.0f));

  glm::vec3 min, max;
  REQUIRE(LsShape::Capsule(1.0f, 0.5f).GetBounds(min, max));
  REQUIRE(Near(max, {0.5f, 1.5f, 0.5f}));
  REQUIRE_FALSE(LsShape::Plane().GetBounds(min, max));
}

TEST_CASE( "scene hierarchy" ) {
  LsScene scene;
  LsNodeId group = scene.AddGroup("Group");
  LsNodeId a = scene.AddShape("A", LsShape::Sphere(1.0f), eRoleConstruction, group);
  LsNodeId b = scene.AddShape("B", LsShape::Sphere(1.0f), eRoleShadowCaster, group);
  LsNodeId c = scene.AddShape("C", LsShape::Sphere(1.0f), eRoleShadowCaster, a);

  REQUIRE(scene.Find(group)->GetParent() == scene.GetRoot());
  REQUIRE(scene.Find(group)->GetChildren() == std::vector<LsNodeId>({a, b}));

  // Cycles are rejected
  REQUIRE_FALSE(scene.Reparent(group, c));
  REQUIRE_FALSE(scene.Reparent(a, a));
  REQUIRE_FALSE(scene.Reparent(scene.GetRoot(), group));

  // Reordering within the same parent
  REQUIRE(scene.Reparent(a, group, 2));
  REQUIRE(scene.Find(group)->GetChildren() == std::vector<LsNodeId>({b, a}));
  REQUIRE(scene.Reparent(a, group, 0));
  REQUIRE(scene.Find(group)->GetChildren() == std::vector<LsNodeId>({a, b}));

  // Duplicate copies the subtree right after the original
  LsNodeId copy = scene.Duplicate(a);
  REQUIRE(scene.Find(group)->GetChildren() == std::vector<LsNodeId>({a, copy, b}));
  REQUIRE(scene.Find(copy)->GetChildren().size() == 1);
  REQUIRE(scene.Find(scene.Find(copy)->GetChildren()[0])->GetProperties().name == "C");

  size_t count = scene.GetNodeCount();
  scene.Remove(copy);
  REQUIRE(scene.GetNodeCount() == count - 2);
  REQUIRE(scene.Find(group)->GetChildren() == std::vector<LsNodeId>({a, b}));

  scene.Remove(group);
  REQUIRE(scene.GetNodeCount() == 1);
  REQUIRE(scene.Find(c) == nullptr);
}

TEST_CASE( "reparent keeps world placement" ) {
  LsScene scene;
  LsNodeId parent = scene.AddGroup("Parent");
  LsPlacement parentPlacement;
  parentPlacement.translation = {5.0f, 0.0f, 0.0f};
  parentPlacement.rotation = {0.0f, 90.0f, 0.0f};
  parentPlacement.scale = glm::vec3(2.0f);
  scene.SetPlacement(parent, parentPlacement);

  LsNodeId child = scene.AddShape("Child", LsShape::Sphere(1.0f), eRoleConstruction);
  LsPlacement childPlacement;
  childPlacement.translation = {1.0f, 2.0f, 3.0f};
  childPlacement.rotation = {10.0f, 20.0f, 30.0f};
  scene.SetPlacement(child, childPlacement);

  glm::mat4 before = scene.GetObjectToWorld(child).GetMatrix();
  uint64_t revision = scene.GetRevision();
  REQUIRE(scene.Reparent(child, parent));
  REQUIRE(scene.GetRevision() > revision);
  REQUIRE(Near(scene.GetObjectToWorld(child).GetMatrix(), before));
  REQUIRE(Near(scene.Find(child)->GetProperties().placement.scale, glm::vec3(0.5f)));

  // Without keeping, the local placement is kept and the node moves with its parent
  REQUIRE(scene.Reparent(child, scene.GetRoot(), SIZE_MAX, false));
  REQUIRE(Near(scene.Find(child)->GetProperties().placement.scale, glm::vec3(0.5f)));
}

TEST_CASE( "flatten" ) {
  LsScene scene;
  LsNodeId group = scene.AddGroup("Group");
  LsPlacement placement;
  placement.translation = {0.0f, 1.0f, 0.0f};
  placement.scale = {2.0f, 2.0f, 4.0f};
  scene.SetPlacement(group, placement);

  LsNodeId light = scene.AddShape("Light", LsShape::Sphere(0.5f), eRoleLightEmitter | eRoleShadowCaster, group);
  LsSceneNodeProperties properties = scene.Find(light)->GetProperties();
  properties.emissionColor = {1.0f, 0.5f, 0.25f};
  properties.emissionIntensity = 4.0f;
  scene.SetProperties(light, properties);

  LsNodeId hidden = scene.AddShape("Hidden", LsShape::Box(glm::vec3(1.0f)), eRoleShadowCaster);
  properties = scene.Find(hidden)->GetProperties();
  properties.visible = false;
  scene.SetProperties(hidden, properties);

  std::vector<LsScenePrimitive> primitives = scene.Flatten();
  REQUIRE(primitives.size() == 1);
  const LsScenePrimitive& primitive = primitives[0];
  REQUIRE(primitive.nodeId == light);
  REQUIRE(primitive.shapeType == static_cast<uint32_t>(LsShapeType::Sphere));
  REQUIRE(primitive.roles == (eRoleLightEmitter | eRoleShadowCaster));
  REQUIRE(Near(glm::vec3(primitive.emission), {4.0f, 2.0f, 1.0f}));
  REQUIRE(primitive.distanceScale == Approx(2.0f));

  // Rows map a world point into object space, as the shader evaluates them
  glm::vec4 world(0.0f, 2.0f, 4.0f, 1.0f);
  glm::vec3 object(glm::dot(primitive.worldToObject[0], world),
                   glm::dot(primitive.worldToObject[1], world),
                   glm::dot(primitive.worldToObject[2], world));
  REQUIRE(Near(object, {0.0f, 0.5f, 1.0f}));
}
