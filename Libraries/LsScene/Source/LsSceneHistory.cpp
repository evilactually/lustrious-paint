#include <LsSceneHistory.h>

LsSceneHistory::LsSceneHistory(std::shared_ptr<LsScene> scene, size_t maxSteps) :
  scene(scene), committed(*scene), committedRevision(scene->GetRevision()), maxSteps(maxSteps) { }

void LsSceneHistory::Update(bool busy) {
  if (busy || scene->GetRevision() == committedRevision) {
    return;
  }
  undoSteps.push_back(committed);
  if (undoSteps.size() > maxSteps) {
    undoSteps.erase(undoSteps.begin());
  }
  // A new change makes the undone ones unreachable
  redoSteps.clear();
  committed = *scene;
  committedRevision = scene->GetRevision();
}

bool LsSceneHistory::CanUndo() const {
  return !undoSteps.empty() || scene->GetRevision() != committedRevision;
}

void LsSceneHistory::Restore(std::vector<LsScene>& from, std::vector<LsScene>& to) {
  Update(false);
  if (from.empty()) {
    return;
  }
  to.push_back(committed);
  committed = from.back();
  from.pop_back();
  scene->Restore(committed);
  committedRevision = scene->GetRevision();
}

void LsSceneHistory::Undo() {
  Restore(undoSteps, redoSteps);
}

void LsSceneHistory::Redo() {
  Restore(redoSteps, undoSteps);
}
