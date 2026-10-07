#pragma once

#include <cstddef>
#include <memory>
#include <LsScene.h>

//-------------------------------------------------------------------------------
// @ LsSceneEditor
//-------------------------------------------------------------------------------
// Dear ImGui panels for building the scene: an outliner showing the node
// hierarchy, and a properties view for placing and configuring the selected node.
//-------------------------------------------------------------------------------
class LsSceneEditor
{
  std::shared_ptr<LsScene> scene;
  LsNodeId selected = kLsInvalidNode;
  // World position new nodes are created at
  glm::vec3 creationPoint = glm::vec3(0.0f);

  // Hierarchy changes requested while the tree is drawn, applied once it's done
  struct {
    LsNodeId node = kLsInvalidNode;
    LsNodeId parent = kLsInvalidNode;
    size_t index = SIZE_MAX;
  } pendingMove;
  LsNodeId pendingRemove = kLsInvalidNode;
  LsNodeId pendingDuplicate = kLsInvalidNode;

  void DrawNode(LsNodeId id);
  void DrawAddPopup();
  void ApplyPendingChanges();
  // Where new nodes go: into the selected group, or next to the selected shape
  LsNodeId GetInsertParent() const;
  // Puts a just added node at the creation point, whatever its parent is
  void PlaceAtCreationPoint(LsNodeId id);
public:
  explicit LsSceneEditor(std::shared_ptr<LsScene> scene);
  LsNodeId GetSelected() const { return selected; }
  // Selection made elsewhere, such as by clicking in the 3D view
  void SetSelected(LsNodeId id) { selected = id; }
  // Where the Add menu creates nodes, in world space, such as the 3D cursor
  void SetCreationPoint(glm::vec3 point) { creationPoint = point; }
  // Draws the "Scene" window
  void DrawOutliner();
  // Draws into the current window
  void DrawProperties();
};
