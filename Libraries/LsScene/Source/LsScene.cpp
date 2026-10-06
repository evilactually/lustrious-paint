#include <LsScene.h>

#include <algorithm>

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
  properties.hasShape = true;
  properties.shape = shape;
  properties.roles = roles;
  return AddNode(properties, Find(parent) ? parent : root);
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
  if (properties.hasShape) {
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
