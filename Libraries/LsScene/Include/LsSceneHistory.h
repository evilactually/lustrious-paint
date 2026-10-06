#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <LsScene.h>

//-------------------------------------------------------------------------------
// @ LsSceneHistory
//-------------------------------------------------------------------------------
// Undo and redo for a scene by whole snapshots, which are small next to the
// canvas. Nothing has to report its edits: Update is called once per frame,
// and when the scene's revision has moved on since the last snapshot and no
// edit is in progress (busy), the change becomes one undo step. A drag that
// changes the scene on every frame is a single step that way.
//-------------------------------------------------------------------------------
class LsSceneHistory
{
  std::shared_ptr<LsScene> scene;
  // The scene as of the last step, what undo goes back from
  LsScene committed;
  uint64_t committedRevision = 0;
  std::vector<LsScene> undoSteps;
  std::vector<LsScene> redoSteps;
  size_t maxSteps;

  void Restore(std::vector<LsScene>& from, std::vector<LsScene>& to);
public:
  explicit LsSceneHistory(std::shared_ptr<LsScene> scene, size_t maxSteps = 256);

  // While busy, changes keep collecting into the step in progress
  void Update(bool busy);
  bool CanUndo() const;
  bool CanRedo() const { return !redoSteps.empty(); }
  // An unfinished change is finished first, so undo always reverts what was last done
  void Undo();
  void Redo();
};
