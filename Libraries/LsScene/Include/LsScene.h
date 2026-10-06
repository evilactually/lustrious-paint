#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <LsCamera.h>
#include <LsShape.h>
#include <LsTransform.h>

using LsNodeId = uint32_t;
const LsNodeId kLsInvalidNode = 0;

// What a shape does in the scene, a shape may have several roles at once.
// Values are shared with Shaders/scene.slang.
enum LsSceneRole : uint32_t
{
  eRoleConstruction      = 1u << 0, // Brush transfers position and normal from it
  eRoleShadowCaster      = 1u << 1, // Occludes light rays towards canvas points
  eRoleLightEmitter      = 1u << 2, // Area light with the node's emission
  eRoleParticipatingMedia = 1u << 3, // Attenuates light passing through the volume
};
using LsSceneRoles = uint32_t;
const LsSceneRoles kLsAllRoles = ~0u;

enum class LsNodeType : uint32_t
{
  Group,  // Only places its children
  Shape,  // Signed distance primitive with roles
  Camera, // Viewpoint the canvas is painted from
};

// Lens of a camera node. The aspect ratio is not part of it, that comes from
// the canvas the camera paints.
struct LsCameraLens
{
  float verticalFov = 40.0f; // in degrees
};

//-------------------------------------------------------------------------------
// @ LsSceneNodeProperties
//-------------------------------------------------------------------------------
// Everything about a node that can be edited freely. The hierarchy itself is
// owned by LsScene so that it can't be made inconsistent.
//-------------------------------------------------------------------------------
struct LsSceneNodeProperties
{
  std::string name;
  LsPlacement placement;
  // A hidden node hides its whole subtree
  bool visible = true;
  LsNodeType type = LsNodeType::Group;
  // Used by shapes only
  LsShape shape;
  LsSceneRoles roles = 0;
  // Used by cameras only
  LsCameraLens lens;
  // Emitted radiance is colour times intensity
  glm::vec3 emissionColor = glm::vec3(1.0f);
  float emissionIntensity = 1.0f;
  glm::vec3 mediumAlbedo = glm::vec3(1.0f);
  float mediumDensity = 1.0f;
};

class LsSceneNode
{
  friend class LsScene;
  LsNodeId id = kLsInvalidNode;
  LsNodeId parent = kLsInvalidNode;
  std::vector<LsNodeId> children;
  LsSceneNodeProperties properties;
public:
  LsNodeId GetId() const { return id; }
  LsNodeId GetParent() const { return parent; }
  const std::vector<LsNodeId>& GetChildren() const { return children; }
  const LsSceneNodeProperties& GetProperties() const { return properties; }
};

//-------------------------------------------------------------------------------
// @ LsScenePrimitive
//-------------------------------------------------------------------------------
// A visible shape with its transform resolved to world space, ready to upload
// as a storage buffer. Layout is std430 compatible and mirrored by
// ScenePrimitive in Shaders/scene.slang.
//-------------------------------------------------------------------------------
struct LsScenePrimitive
{
  // Rows of the 3x4 affine world-to-object matrix
  glm::vec4 worldToObject[3];
  glm::vec4 shapeParams;
  // rgb = emitted radiance
  glm::vec4 emission;
  // rgb = albedo, a = density
  glm::vec4 medium;
  uint32_t shapeType;
  LsSceneRoles roles;
  // Object space distances times this are safe world space distances
  float distanceScale;
  LsNodeId nodeId;
};
static_assert(sizeof(LsScenePrimitive) == 112, "LsScenePrimitive must match ScenePrimitive in scene.slang");

struct LsSceneHit
{
  LsNodeId node = kLsInvalidNode;
  float distance = 0.0f; // along the ray
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
};

// Sphere traces the union of primitives having any role in roleMask. Primitives
// containing the ray origin are skipped, so a ray from inside a room still hits
// its walls and a camera below the ground still sees what is above it.
bool LsRaycastPrimitives(const std::vector<LsScenePrimitive>& primitives, const LsRay& ray,
                         LsSceneRoles roleMask, float maxDistance, LsSceneHit& hit);

//-------------------------------------------------------------------------------
// @ LsScene
//-------------------------------------------------------------------------------
// Hierarchy of placed shapes. Where pbrt nests transforms with an attribute
// stack while parsing and then flattens the result into a list of primitives,
// here the nesting is kept as a tree of nodes so it can be edited, and
// Flatten() produces the equivalent primitive list for rendering.
//
// Every change bumps the revision, so renderers can tell when to re-upload.
//-------------------------------------------------------------------------------
class LsScene
{
  std::unordered_map<LsNodeId, LsSceneNode> nodes;
  LsNodeId root = kLsInvalidNode;
  LsNodeId nextId = 1;
  LsNodeId activeCamera = kLsInvalidNode;
  uint64_t revision = 0;

  LsNodeId AddNode(const LsSceneNodeProperties& properties, LsNodeId parent);
  void FlattenNode(LsNodeId id, const LsTransform& parentToWorld, std::vector<LsScenePrimitive>& primitives) const;
public:
  LsScene();

  LsNodeId GetRoot() const { return root; }
  uint64_t GetRevision() const { return revision; }
  size_t GetNodeCount() const { return nodes.size(); }

  // Returns nullptr for removed or unknown ids
  const LsSceneNode* Find(LsNodeId id) const;
  // Parent defaults to the root
  LsNodeId AddGroup(const std::string& name, LsNodeId parent = kLsInvalidNode);
  LsNodeId AddShape(const std::string& name, const LsShape& shape, LsSceneRoles roles, LsNodeId parent = kLsInvalidNode);
  // The first camera added becomes the active one
  LsNodeId AddCamera(const std::string& name, const LsCameraLens& lens = LsCameraLens(), LsNodeId parent = kLsInvalidNode);
  // Copies the node and its subtree next to the original, returns the copy
  LsNodeId Duplicate(LsNodeId id);
  // Removes the node and its subtree, the root can't be removed
  void Remove(LsNodeId id);

  void SetProperties(LsNodeId id, const LsSceneNodeProperties& properties);
  void SetPlacement(LsNodeId id, const LsPlacement& placement);

  // Moves a node under a new parent, inserting it before the child at index,
  // or at the end if index is out of range. Fails when it would create a cycle.
  // With keepWorldPlacement the node stays where it is in world space.
  bool Reparent(LsNodeId id, LsNodeId newParent, size_t index = SIZE_MAX, bool keepWorldPlacement = true);

  // True if ancestor is id itself or one of its parents
  bool IsAncestor(LsNodeId ancestor, LsNodeId id) const;

  LsTransform GetObjectToWorld(LsNodeId id) const;

  // World space box around the node's subtree. Unbounded shapes such as planes
  // only add their origin, cameras add their position. False for empty groups.
  bool GetWorldBounds(LsNodeId id, glm::vec3& min, glm::vec3& max) const;

  // The camera the canvas is painted from. When the active camera is removed
  // the first remaining camera in depth-first order takes over.
  LsNodeId GetActiveCamera() const;
  void SetActiveCamera(LsNodeId id);
  // View through a camera node, aspect is width over height of the canvas
  LsCamera GetCamera(LsNodeId id, float aspect) const;

  // All visible shapes in depth-first order
  std::vector<LsScenePrimitive> Flatten() const;

  // Flattens and traces the scene, see LsRaycastPrimitives
  bool Raycast(const LsRay& ray, LsSceneHit& hit, LsSceneRoles roleMask = kLsAllRoles, float maxDistance = 1000.0f) const;
};
