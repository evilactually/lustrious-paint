#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <LsScene.h>
#include <LsSceneHistory.h>

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
  REQUIRE(LsShape::Plane(1.0f).Distance({0.5f, 2.0f, 0.5f}) == Approx(2.0f));
  // Two-sided, and beyond the edge it is the distance to the edge
  REQUIRE(LsShape::Plane(1.0f).Distance({0.5f, -2.0f, 0.5f}) == Approx(2.0f));
  REQUIRE(LsShape::Plane(1.0f).Distance({4.0f, 4.0f, 0.0f}) == Approx(5.0f));

  glm::vec3 min, max;
  REQUIRE(LsShape::Capsule(1.0f, 0.5f).GetBounds(min, max));
  REQUIRE(Near(max, {0.5f, 1.5f, 0.5f}));
  REQUIRE(LsShape::Plane(2.0f).GetBounds(min, max));
  REQUIRE(Near(max, {2.0f, 0.0f, 2.0f}));
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

TEST_CASE( "camera rays and projection" ) {
  LsCamera camera = LsCamera::LookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 90.0f, 2.0f);
  REQUIRE(Near(camera.GetForward(), {0.0f, 0.0f, -1.0f}));
  REQUIRE(Near(camera.GetRight(), {1.0f, 0.0f, 0.0f}));
  REQUIRE(camera.GetVerticalFov() == Approx(90.0f));

  // 90 degrees vertically means the top edge of the image is at 45 degrees
  LsRay top = camera.GenerateRay({0.0f, 1.0f});
  REQUIRE(Near(top.direction, glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f))));
  // The aspect widens the horizontal field of view
  LsRay right = camera.GenerateRay({1.0f, 0.0f});
  REQUIRE(Near(right.direction, glm::normalize(glm::vec3(2.0f, 0.0f, -1.0f))));

  glm::vec2 ndc;
  REQUIRE(camera.Project(camera.GenerateRay({0.3f, -0.6f}).At(7.0f), ndc));
  REQUIRE(Near(glm::vec3(ndc, 0.0f), {0.3f, -0.6f, 0.0f}));
  REQUIRE_FALSE(camera.Project({0.0f, 0.0f, 6.0f}, ndc));
  REQUIRE(camera.GetDepth({1.0f, 1.0f, 2.0f}) == Approx(3.0f));
  REQUIRE(Near(camera.GetImagePlanePoint({1.0f, 1.0f}, 1.0f), {2.0f, 1.0f, 4.0f}));

  // Looking straight down still gives a valid frame
  LsCamera down = LsCamera::LookAt({0.0f, 5.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 40.0f, 1.0f);
  REQUIRE(Near(down.GetForward(), {0.0f, -1.0f, 0.0f}));
  REQUIRE(std::abs(glm::dot(down.GetRight(), down.GetUp())) < 1.0e-5f);

  // The frame survives a round trip through a matrix, scale is dropped
  LsCamera copy(camera.GetCameraToWorld()*glm::mat4(glm::mat3(3.0f)), 90.0f, 2.0f);
  REQUIRE(Near(copy.GetForward(), camera.GetForward()));
  REQUIRE(Near(copy.GetUp(), camera.GetUp()));
  REQUIRE(Near(copy.GetPosition(), camera.GetPosition()));
}

TEST_CASE( "camera nodes" ) {
  LsScene scene;
  REQUIRE(scene.GetActiveCamera() == kLsInvalidNode);
  LsNodeId first = scene.AddCamera("First");
  LsNodeId second = scene.AddCamera("Second");
  REQUIRE(scene.GetActiveCamera() == first);
  scene.SetActiveCamera(second);
  REQUIRE(scene.GetActiveCamera() == second);
  // Only cameras can be active
  LsNodeId shape = scene.AddShape("Shape", LsShape::Sphere(1.0f), eRoleConstruction);
  scene.SetActiveCamera(shape);
  REQUIRE(scene.GetActiveCamera() == second);
  scene.Remove(second);
  REQUIRE(scene.GetActiveCamera() == first);

  // Cameras are not primitives
  REQUIRE(scene.Flatten().size() == 1);

  // A camera node with no rotation looks down -Z
  LsPlacement placement;
  placement.translation = {1.0f, 2.0f, 3.0f};
  placement.rotation = {0.0f, 90.0f, 0.0f};
  scene.SetPlacement(first, placement);
  LsCamera camera = scene.GetCamera(first, 1.5f);
  REQUIRE(Near(camera.GetPosition(), {1.0f, 2.0f, 3.0f}));
  REQUIRE(Near(camera.GetForward(), {-1.0f, 0.0f, 0.0f}));
  REQUIRE(camera.GetAspect() == Approx(1.5f));
  REQUIRE(camera.GetVerticalFov() == Approx(LsCameraLens().verticalFov));
}

TEST_CASE( "raycast" ) {
  LsScene scene;
  LsNodeId ground = scene.AddShape("Ground", LsShape::Plane(10.0f), eRoleConstruction);
  LsNodeId sphere = scene.AddShape("Sphere", LsShape::Sphere(1.0f), eRoleShadowCaster);
  LsPlacement placement;
  placement.translation = {0.0f, 1.0f, 0.0f};
  placement.scale = {2.0f, 1.0f, 1.0f};
  scene.SetPlacement(sphere, placement);

  LsRay ray;
  ray.origin = {-10.0f, 1.0f, 0.0f};
  ray.direction = {1.0f, 0.0f, 0.0f};
  LsSceneHit hit;
  REQUIRE(scene.Raycast(ray, hit));
  REQUIRE(hit.node == sphere);
  REQUIRE(hit.distance == Approx(8.0f).epsilon(1.0e-3));
  REQUIRE(Near(hit.normal, {-1.0f, 0.0f, 0.0f}, 1.0e-2f));

  // Roles filter what can be hit
  REQUIRE_FALSE(scene.Raycast(ray, hit, eRoleConstruction));

  ray.origin = {5.0f, 3.0f, 0.0f};
  ray.direction = glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f));
  REQUIRE(scene.Raycast(ray, hit));
  REQUIRE(hit.node == ground);
  REQUIRE(Near(hit.position, {5.0f, 0.0f, -3.0f}, 1.0e-2f));
  REQUIRE(Near(hit.normal, {0.0f, 1.0f, 0.0f}, 1.0e-3f));

  // The plane is two-sided and ends at its edges
  ray.origin = {5.0f, -3.0f, 0.0f};
  ray.direction = glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f));
  REQUIRE(scene.Raycast(ray, hit));
  REQUIRE(hit.node == ground);
  REQUIRE(Near(hit.normal, {0.0f, -1.0f, 0.0f}, 1.0e-3f));
  ray.origin = {20.0f, 3.0f, 0.0f};
  ray.direction = {0.0f, -1.0f, 0.0f};
  REQUIRE_FALSE(scene.Raycast(ray, hit));

  // From inside the sphere the sphere itself is ignored
  ray.origin = {0.0f, 1.0f, 0.0f};
  ray.direction = {0.0f, -1.0f, 0.0f};
  REQUIRE(scene.Raycast(ray, hit));
  REQUIRE(hit.node == ground);

  // Missing everything
  ray.origin = {0.0f, 5.0f, 0.0f};
  ray.direction = {0.0f, 1.0f, 0.0f};
  REQUIRE_FALSE(scene.Raycast(ray, hit));
}

TEST_CASE( "world bounds" ) {
  LsScene scene;
  LsNodeId group = scene.AddGroup("Group");
  glm::vec3 min, max;
  REQUIRE_FALSE(scene.GetWorldBounds(group, min, max));

  LsPlacement placement;
  placement.translation = {10.0f, 0.0f, 0.0f};
  scene.SetPlacement(group, placement);
  LsNodeId box = scene.AddShape("Box", LsShape::Box({1.0f, 2.0f, 3.0f}), eRoleConstruction, group);
  placement.translation = {0.0f, 5.0f, 0.0f};
  placement.rotation = {0.0f, 90.0f, 0.0f};
  scene.SetPlacement(box, placement);
  REQUIRE(scene.GetWorldBounds(group, min, max));
  // Rotated a quarter turn about Y, so X and Z extents swap
  REQUIRE(Near(min, {7.0f, 3.0f, -1.0f}));
  REQUIRE(Near(max, {13.0f, 7.0f, 1.0f}));

  // Cameras contribute their position, planes their square
  LsNodeId camera = scene.AddCamera("Camera", LsCameraLens(), group);
  placement = LsPlacement();
  placement.translation = {0.0f, 0.0f, 20.0f};
  scene.SetPlacement(camera, placement);
  scene.AddShape("Plane", LsShape::Plane(1.0f), eRoleConstruction, group);
  REQUIRE(scene.GetWorldBounds(group, min, max));
  REQUIRE(Near(min, {7.0f, 0.0f, -1.0f}));
  REQUIRE(Near(max, {13.0f, 7.0f, 20.0f}));
}

TEST_CASE( "placement from matrix with a hint" ) {
  LsPlacement hint;
  hint.translation = {1.0f, 2.0f, 3.0f};
  hint.rotation = {170.0f, 10.0f, -170.0f};
  hint.scale = {2.0f, 1.0f, 0.5f};

  // Moving keeps rotation and scale bit for bit
  LsPlacement moved = hint;
  moved.translation = {4.0f, 5.0f, 6.0f};
  LsPlacement decomposed = LsPlacement::FromMatrix(moved.ToTransform().GetMatrix(), hint);
  REQUIRE(decomposed.rotation == hint.rotation);
  REQUIRE(decomposed.scale == hint.scale);
  REQUIRE(Near(decomposed.translation, moved.translation));

  // The plain decomposition finds the other Euler solution, the hint keeps the original
  LsPlacement rotated = hint;
  rotated.rotation.y += 5.0f;
  glm::mat4 m = rotated.ToTransform().GetMatrix();
  decomposed = LsPlacement::FromMatrix(m, hint);
  REQUIRE(Near(decomposed.rotation, rotated.rotation, 1.0e-2f));
  REQUIRE(Near(decomposed.ToTransform().GetMatrix(), m));

  // Whole turns are kept
  hint.rotation = {0.0f, 0.0f, 350.0f};
  rotated = hint;
  rotated.rotation.z = 365.0f;
  decomposed = LsPlacement::FromMatrix(rotated.ToTransform().GetMatrix(), hint);
  REQUIRE(decomposed.rotation.z == Approx(365.0f).epsilon(1.0e-4));
}

TEST_CASE( "restore keeps revisions and ids increasing" ) {
  LsScene scene;
  LsScene snapshot = scene;
  LsNodeId added = scene.AddShape("Added", LsShape::Sphere(1.0f), eRoleConstruction);
  uint64_t revision = scene.GetRevision();
  scene.Restore(snapshot);
  REQUIRE(scene.Find(added) == nullptr);
  REQUIRE(scene.GetRevision() > revision);
  // The undone node's id isn't handed out again
  REQUIRE(scene.AddGroup("Group") != added);
}

TEST_CASE( "history" ) {
  auto scene = std::make_shared<LsScene>();
  LsSceneHistory history(scene);
  REQUIRE_FALSE(history.CanUndo());

  LsNodeId sphere = scene->AddShape("Sphere", LsShape::Sphere(1.0f), eRoleConstruction);
  history.Update(false);

  // A drag changes the scene on many frames but is one step
  LsPlacement placement;
  for (int frame = 1; frame <= 10; frame++) {
    placement.translation.x = float(frame);
    scene->SetPlacement(sphere, placement);
    history.Update(true);
  }
  history.Update(false);
  REQUIRE(history.CanUndo());

  history.Undo();
  REQUIRE(scene->Find(sphere)->GetProperties().placement.translation.x == 0.0f);
  history.Undo();
  REQUIRE(scene->Find(sphere) == nullptr);
  REQUIRE_FALSE(history.CanUndo());
  // Restoring isn't mistaken for a new change
  history.Update(false);
  REQUIRE(history.CanRedo());

  history.Redo();
  history.Redo();
  REQUIRE(scene->Find(sphere)->GetProperties().placement.translation.x == 10.0f);
  REQUIRE_FALSE(history.CanRedo());

  // Undo finishes a change still in progress before reverting it
  placement.translation.x = 20.0f;
  scene->SetPlacement(sphere, placement);
  history.Undo();
  REQUIRE(scene->Find(sphere)->GetProperties().placement.translation.x == 10.0f);

  // A new change drops what was undone
  history.Undo();
  scene->AddGroup("Group");
  history.Update(false);
  REQUIRE_FALSE(history.CanRedo());
}

TEST_CASE( "camera lens shift" ) {
  LsCamera camera = LsCamera::LookAt({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, 90.0f, 1.0f);
  camera.SetLensShift({0.5f, -0.25f});
  // The image centre looks off the axis by the shift
  REQUIRE(Near(camera.GenerateRay({0.0f, 0.0f}).direction, glm::normalize(glm::vec3(0.5f, -0.25f, -1.0f))));
  REQUIRE(Near(camera.GetImagePlanePoint({1.0f, 1.0f}, 2.0f), {3.0f, 1.5f, -2.0f}));
  glm::vec2 ndc;
  REQUIRE(camera.Project(camera.GenerateRay({0.3f, 0.7f}).At(5.0f), ndc));
  REQUIRE(Near(glm::vec3(ndc, 0.0f), {0.3f, 0.7f, 0.0f}));
  // The forward axis itself lands off centre
  REQUIRE(camera.Project({0.0f, 0.0f, -1.0f}, ndc));
  REQUIRE(Near(glm::vec3(ndc, 0.0f), {-0.5f, 0.25f, 0.0f}));
}
