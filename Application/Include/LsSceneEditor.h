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
public:
  explicit LsSceneEditor(std::shared_ptr<LsScene> scene);
  LsNodeId GetSelected() const { return selected; }
  // Selection made elsewhere, such as by clicking in the 3D view
  void SetSelected(LsNodeId id) { selected = id; }
  // Draws the "Scene" window
  void DrawOutliner();
  // Draws into the current window
  void DrawProperties();
};
