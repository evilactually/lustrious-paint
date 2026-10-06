#include <LsScene.h>

#include <algorithm>
#include <cfloat>

LsScene::LsScene() {
  LsSceneNodeProperties properties;
  properties.name = "Scene";
  root = AddNode(properties, kLsInvalidNode);
}

LsNodeId LsScene::AddNode(const LsSceneNodeProperties& properties, LsNodeId parent) {
  LsNodeId id = nextId++;
  LsSceneNode& node = nodes[id];
  node.id = id;
  node.parent = parent;
  node.properties = properties;
  if (parent != kLsInvalidNode) {
    nodes.at(parent).children.push_back(id);
  }
  revision++;
  return id;
}

const LsSceneNode* LsScene::Find(LsNodeId id) const {
  auto it = nodes.find(id);
  return it != nodes.end() ? &it->second : nullptr;
}

LsNodeId LsScene::AddGroup(const std::string& name, LsNodeId parent) {
  LsSceneNodeProperties properties;
  properties.name = name;
  return AddNode(properties, Find(parent) ? parent : root);
}

LsNodeId LsScene::AddShape(const std::string& name, const LsShape& shape, LsSceneRoles roles, LsNodeId parent) {
  LsSceneNodeProperties properties;
  properties.name = name;
  properties.type = LsNodeType::Shape;
  properties.shape = shape;
  properties.roles = roles;
  return AddNode(properties, Find(parent) ? parent : root);
}

LsNodeId LsScene::AddCamera(const std::string& name, const LsCameraLens& lens, LsNodeId parent) {
  LsSceneNodeProperties properties;
  properties.name = name;
  properties.type = LsNodeType::Camera;
  properties.lens = lens;
  LsNodeId id = AddNode(properties, Find(parent) ? parent : root);
  if (GetActiveCamera() == id) {
    activeCamera = id;
  }
  return id;
}

LsNodeId LsScene::Duplicate(LsNodeId id) {
  const LsSceneNode* node = Find(id);
  if (!node || id == root) {
    return kLsInvalidNode;
  }

  // Walk the source subtree with an explicit stack, each entry knows where its copy goes
  struct Copy { LsNodeId source; LsNodeId parent; };
  std::vector<Copy> pending = { { id, node->parent } };
  LsNodeId duplicate = kLsInvalidNode;
  while (!pending.empty()) {
    Copy copy = pending.back();
    pending.pop_back();
    LsNodeId copyId = AddNode(nodes.at(copy.source).properties, copy.parent);
    if (duplicate == kLsInvalidNode) {
      duplicate = copyId;
    }
    const std::vector<LsNodeId>& children = nodes.at(copy.source).children;
    // Reverse so the stack pops children in their original order
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
      pending.push_back({ *it, copyId });
    }
  }

  // Place the copy right after the original rather than at the end
  std::vector<LsNodeId>& siblings = nodes.at(nodes.at(id).parent).children;
  siblings.pop_back();
  siblings.insert(std::find(siblings.begin(), siblings.end(), id) + 1, duplicate);
  return duplicate;
}

void LsScene::Remove(LsNodeId id) {
  const LsSceneNode* node = Find(id);
  if (!node || id == root) {
    return;
  }

  std::vector<LsNodeId>& siblings = nodes.at(node->parent).children;
  siblings.erase(std::remove(siblings.begin(), siblings.end(), id), siblings.end());

  std::vector<LsNodeId> pending = { id };
  while (!pending.empty()) {
    LsNodeId current = pending.back();
    pending.pop_back();
    const std::vector<LsNodeId>& children = nodes.at(current).children;
    pending.insert(pending.end(), children.begin(), children.end());
    nodes.erase(current);
  }
  revision++;
}

void LsScene::SetProperties(LsNodeId id, const LsSceneNodeProperties& properties) {
  auto it = nodes.find(id);
  if (it != nodes.end()) {
    it->second.properties = properties;
    revision++;
  }
}

void LsScene::SetPlacement(LsNodeId id, const LsPlacement& placement) {
  auto it = nodes.find(id);
  if (it != nodes.end()) {
    it->second.properties.placement = placement;
    revision++;
  }
}

bool LsScene::IsAncestor(LsNodeId ancestor, LsNodeId id) const {
  for (const LsSceneNode* node = Find(id); node; node = Find(node->parent)) {
    if (node->id == ancestor) {
      return true;
    }
  }
  return false;
}

bool LsScene::Reparent(LsNodeId id, LsNodeId newParent, size_t index, bool keepWorldPlacement) {
  if (id == root || !Find(id) || !Find(newParent) || IsAncestor(id, newParent)) {
    return false;
  }

  LsSceneNode& node = nodes.at(id);
  if (keepWorldPlacement) {
    LsTransform objectToParent = GetObjectToWorld(newParent).Inverse()*GetObjectToWorld(id);
    node.properties.placement = LsPlacement::FromMatrix(objectToParent.GetMatrix());
  }

  std::vector<LsNodeId>& oldSiblings = nodes.at(node.parent).children;
  auto position = std::find(oldSiblings.begin(), oldSiblings.end(), id);
  // Moving later within the same parent, the removal shifts the target index down
  if (node.parent == newParent && index != SIZE_MAX && static_cast<size_t>(position - oldSiblings.begin()) < index) {
    index--;
  }
  oldSiblings.erase(position);

  std::vector<LsNodeId>& newSiblings = nodes.at(newParent).children;
  newSiblings.insert(newSiblings.begin() + std::min(index, newSiblings.size()), id);
  node.parent = newParent;
  revision++;
  return true;
}

LsTransform LsScene::GetObjectToWorld(LsNodeId id) const {
  LsTransform objectToWorld;
  for (const LsSceneNode* node = Find(id); node; node = Find(node->parent)) {
    objectToWorld = node->properties.placement.ToTransform()*objectToWorld;
  }
  return objectToWorld;
}

bool LsScene::GetWorldBounds(LsNodeId id, glm::vec3& min, glm::vec3& max) const {
  if (!Find(id)) {
    return false;
  }
  bool found = false;
  auto add = [&](glm::vec3 p) {
    min = found ? glm::min(min, p) : p;
    max = found ? glm::max(max, p) : p;
    found = true;
  };

  struct Pending { LsNodeId id; LsTransform objectToWorld; };
  std::vector<Pending> pending = { { id, GetObjectToWorld(id) } };
  while (!pending.empty()) {
    Pending current = pending.back();
    pending.pop_back();
    const LsSceneNode& node = nodes.at(current.id);
    const LsSceneNodeProperties& properties = node.properties;

    glm::vec3 objectMin, objectMax;
    if (properties.type == LsNodeType::Shape && properties.shape.GetBounds(objectMin, objectMax)) {
      for (int corner = 0; corner < 8; corner++) {
        glm::vec3 p((corner & 1) ? objectMax.x : objectMin.x,
                    (corner & 2) ? objectMax.y : objectMin.y,
                    (corner & 4) ? objectMax.z : objectMin.z);
        add(current.objectToWorld.ApplyToPoint(p));
      }
    }
    else if (properties.type != LsNodeType::Group) {
      add(current.objectToWorld.ApplyToPoint(glm::vec3(0.0f)));
    }

    for (LsNodeId child : node.children) {
      pending.push_back({ child, current.objectToWorld*nodes.at(child).properties.placement.ToTransform() });
    }
  }
  return found;
}

LsNodeId LsScene::GetActiveCamera() const {
  const LsSceneNode* active = Find(activeCamera);
  if (active && active->properties.type == LsNodeType::Camera) {
    return activeCamera;
  }
  std::vector<LsNodeId> pending = { root };
  while (!pending.empty()) {
    const LsSceneNode& node = nodes.at(pending.back());
    pending.pop_back();
    if (node.properties.type == LsNodeType::Camera) {
      return node.id;
    }
    pending.insert(pending.end(), node.children.rbegin(), node.children.rend());
  }
  return kLsInvalidNode;
}

void LsScene::SetActiveCamera(LsNodeId id) {
  const LsSceneNode* node = Find(id);
  if (node && node->properties.type == LsNodeType::Camera && activeCamera != id) {
    activeCamera = id;
    revision++;
  }
}

LsCamera LsScene::GetCamera(LsNodeId id, float aspect) const {
  const LsSceneNode* node = Find(id);
  float fov = node ? node->properties.lens.verticalFov : LsCameraLens().verticalFov;
  return LsCamera(GetObjectToWorld(id).GetMatrix(), fov, aspect);
}

void LsScene::Restore(const LsScene& snapshot) {
  uint64_t nextRevision = std::max(revision, snapshot.revision) + 1;
  LsNodeId unusedId = std::max(nextId, snapshot.nextId);
  *this = snapshot;
  revision = nextRevision;
  nextId = unusedId;
}

bool LsScene::Raycast(const LsRay& ray, LsSceneHit& hit, LsSceneRoles roleMask, float maxDistance) const {
  return LsRaycastPrimitives(Flatten(), ray, roleMask, maxDistance, hit);
}

static float PrimitiveDistance(const LsScenePrimitive& primitive, glm::vec3 p) {
  glm::vec4 h(p, 1.0f);
  glm::vec3 objectP(glm::dot(primitive.worldToObject[0], h),
                    glm::dot(primitive.worldToObject[1], h),
                    glm::dot(primitive.worldToObject[2], h));
  LsShape shape = { static_cast<LsShapeType>(primitive.shapeType), primitive.shapeParams };
  return shape.Distance(objectP)*primitive.distanceScale;
}

bool LsRaycastPrimitives(const std::vector<LsScenePrimitive>& primitives, const LsRay& ray,
                         LsSceneRoles roleMask, float maxDistance, LsSceneHit& hit) {
  std::vector<const LsScenePrimitive*> candidates;
  for (const LsScenePrimitive& primitive : primitives) {
    if ((primitive.roles & roleMask) && PrimitiveDistance(primitive, ray.origin) > 0.0f) {
      candidates.push_back(&primitive);
    }
  }

  const int kMaxSteps = 512;
  float t = 0.0f;
  for (int step = 0; step < kMaxSteps && t < maxDistance; step++) {
    glm::vec3 p = ray.At(t);
    float distance = FLT_MAX;
    const LsScenePrimitive* closest = nullptr;
    for (const LsScenePrimitive* primitive : candidates) {
      float d = PrimitiveDistance(*primitive, p);
      if (d < distance) {
        distance = d;
        closest = primitive;
      }
    }
    if (!closest) {
      return false;
    }

    // Tolerance grows with distance, far away a pixel covers more
    float epsilon = 1.0e-4f*std::max(1.0f, t);
    if (distance < epsilon) {
      // Central differences of the hit primitive alone, the union would blur normals where shapes touch
      float h = epsilon;
      glm::vec3 gradient(PrimitiveDistance(*closest, p + glm::vec3(h, 0.0f, 0.0f)) - PrimitiveDistance(*closest, p - glm::vec3(h, 0.0f, 0.0f)),
                         PrimitiveDistance(*closest, p + glm::vec3(0.0f, h, 0.0f)) - PrimitiveDistance(*closest, p - glm::vec3(0.0f, h, 0.0f)),
                         PrimitiveDistance(*closest, p + glm::vec3(0.0f, 0.0f, h)) - PrimitiveDistance(*closest, p - glm::vec3(0.0f, 0.0f, h)));
      float length = glm::length(gradient);
      hit.node = closest->nodeId;
      hit.distance = t;
      hit.position = p;
      hit.normal = length > 0.0f ? gradient/length : -ray.direction;
      return true;
    }
    t += distance;
  }
  return false;
}

std::vector<LsScenePrimitive> LsScene::Flatten() const {
  std::vector<LsScenePrimitive> primitives;
  FlattenNode(root, LsTransform(), primitives);
  return primitives;
}

void LsScene::FlattenNode(LsNodeId id, const LsTransform& parentToWorld, std::vector<LsScenePrimitive>& primitives) const {
  const LsSceneNode& node = nodes.at(id);
  const LsSceneNodeProperties& properties = node.properties;
  if (!properties.visible) {
    return;
  }

  LsTransform objectToWorld = parentToWorld*properties.placement.ToTransform();
  if (properties.type == LsNodeType::Shape) {
    LsScenePrimitive primitive = {};
    glm::mat4 worldToObject = glm::transpose(objectToWorld.GetInverseMatrix());
    for (int row = 0; row < 3; row++) {
      primitive.worldToObject[row] = worldToObject[row];
    }
    primitive.shapeParams = properties.shape.params;
    primitive.emission = glm::vec4(properties.emissionColor*properties.emissionIntensity, 0.0f);
    primitive.medium = glm::vec4(properties.mediumAlbedo, properties.mediumDensity);
    primitive.shapeType = static_cast<uint32_t>(properties.shape.type);
    primitive.roles = properties.roles;
    primitive.distanceScale = objectToWorld.GetMinScale();
    primitive.nodeId = id;
    primitives.push_back(primitive);
  }

  for (LsNodeId child : node.children) {
    FlattenNode(child, objectToWorld, primitives);
  }
}
