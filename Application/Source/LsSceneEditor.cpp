#include <LsSceneEditor.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <imgui.h>

static const char* kNodePayload = "LS_SCENE_NODE";

LsSceneEditor::LsSceneEditor(std::shared_ptr<LsScene> scene) : scene(scene) { }

LsNodeId LsSceneEditor::GetInsertParent() const {
  const LsSceneNode* node = scene->Find(selected);
  if (!node) {
    return scene->GetRoot();
  }
  return node->GetProperties().type == LsNodeType::Group ? selected : node->GetParent();
}

void LsSceneEditor::DrawOutliner() {
  if (!ImGui::Begin("Scene")) {
    ImGui::End();
    return;
  }

  bool hasSelection = scene->Find(selected) != nullptr;
  if (ImGui::Button("Add")) {
    ImGui::OpenPopup("AddNode");
  }
  DrawAddPopup();
  ImGui::SameLine();
  ImGui::BeginDisabled(!hasSelection);
  if (ImGui::Button("Duplicate")) {
    pendingDuplicate = selected;
  }
  ImGui::SameLine();
  if (ImGui::Button("Delete")) {
    pendingRemove = selected;
  }
  ImGui::EndDisabled();

  // Shortcuts only apply while the outliner has focus
  if (hasSelection && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
      pendingRemove = selected;
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_D)) {
      pendingDuplicate = selected;
    }
  }

  ImGui::Separator();
  for (LsNodeId child : scene->Find(scene->GetRoot())->GetChildren()) {
    DrawNode(child);
  }

  // The space below the tree deselects on click and moves dropped nodes to the top level
  ImVec2 rest = ImGui::GetContentRegionAvail();
  ImGui::InvisibleButton("##Background", ImVec2(std::max(rest.x, 1.0f), std::max(rest.y, ImGui::GetFrameHeight())));
  if (ImGui::IsItemClicked()) {
    selected = kLsInvalidNode;
  }
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kNodePayload)) {
      pendingMove.node = *static_cast<const LsNodeId*>(payload->Data);
      pendingMove.parent = scene->GetRoot();
      pendingMove.index = SIZE_MAX;
    }
    ImGui::EndDragDropTarget();
  }

  ImGui::End();
  ApplyPendingChanges();
}

void LsSceneEditor::DrawAddPopup() {
  if (!ImGui::BeginPopup("AddNode")) {
    return;
  }
  LsNodeId added = kLsInvalidNode;
  if (ImGui::MenuItem("Group")) {
    added = scene->AddGroup("Group", GetInsertParent());
  }
  if (ImGui::MenuItem("Camera")) {
    added = scene->AddCamera("Camera", LsCameraLens(), GetInsertParent());
  }
  ImGui::Separator();
  for (uint32_t i = 0; i < static_cast<uint32_t>(LsShapeType::Count); i++) {
    LsShapeType type = static_cast<LsShapeType>(i);
    if (ImGui::MenuItem(LsShapeTypeName(type))) {
      added = scene->AddShape(LsShapeTypeName(type), LsShape::Default(type), eRoleConstruction, GetInsertParent());
    }
  }
  if (added != kLsInvalidNode) {
    selected = added;
  }
  ImGui::EndPopup();
}

void LsSceneEditor::DrawNode(LsNodeId id) {
  const LsSceneNode* node = scene->Find(id);
  const LsSceneNodeProperties& properties = node->GetProperties();

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanAvailWidth |
                             ImGuiTreeNodeFlags_DefaultOpen;
  if (node->GetChildren().empty()) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }
  if (id == selected) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  ImGui::PushID(static_cast<int>(id));
  if (!properties.visible) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
  }
  bool open = ImGui::TreeNodeEx("##Node", flags, "%s", properties.name.c_str());
  if (!properties.visible) {
    ImGui::PopStyleColor();
  }

  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    selected = id;
  }

  if (ImGui::BeginDragDropSource()) {
    ImGui::SetDragDropPayload(kNodePayload, &id, sizeof(id));
    ImGui::TextUnformatted(properties.name.c_str());
    ImGui::EndDragDropSource();
  }

  // Dropping on the top edge of a row inserts before it, anywhere else makes it a child
  if (ImGui::BeginDragDropTarget()) {
    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kNodePayload, ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                             ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
    if (payload) {
      ImVec2 min = ImGui::GetItemRectMin();
      ImVec2 max = ImGui::GetItemRectMax();
      bool before = ImGui::GetMousePos().y < min.y + (max.y - min.y)*0.3f;
      ImU32 color = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
      if (before) {
        ImGui::GetWindowDrawList()->AddLine(min, ImVec2(max.x, min.y), color, 2.0f);
      }
      else {
        ImGui::GetWindowDrawList()->AddRect(min, max, color, 0.0f, 0, 2.0f);
      }

      if (payload->IsDelivery()) {
        pendingMove.node = *static_cast<const LsNodeId*>(payload->Data);
        if (before) {
          const std::vector<LsNodeId>& siblings = scene->Find(node->GetParent())->GetChildren();
          pendingMove.parent = node->GetParent();
          pendingMove.index = std::find(siblings.begin(), siblings.end(), id) - siblings.begin();
        }
        else {
          pendingMove.parent = id;
          pendingMove.index = SIZE_MAX;
        }
      }
    }
    ImGui::EndDragDropTarget();
  }

  // After the drag and drop calls, they act on the last item
  if (id == scene->GetActiveCamera()) {
    ImGui::SameLine();
    ImGui::TextDisabled("(canvas)");
  }

  if (open) {
    for (LsNodeId child : node->GetChildren()) {
      DrawNode(child);
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
}

void LsSceneEditor::ApplyPendingChanges() {
  if (pendingMove.node != kLsInvalidNode) {
    // Fails harmlessly when dropping a node onto its own subtree
    scene->Reparent(pendingMove.node, pendingMove.parent, pendingMove.index);
    pendingMove.node = kLsInvalidNode;
  }
  if (pendingDuplicate != kLsInvalidNode) {
    selected = scene->Duplicate(pendingDuplicate);
    pendingDuplicate = kLsInvalidNode;
  }
  if (pendingRemove != kLsInvalidNode) {
    scene->Remove(pendingRemove);
    if (!scene->Find(selected)) {
      selected = kLsInvalidNode;
    }
    pendingRemove = kLsInvalidNode;
  }
}

void LsSceneEditor::DrawProperties() {
  const LsSceneNode* node = scene->Find(selected);
  if (!node) {
    ImGui::TextDisabled("Select an object in the Scene panel");
    return;
  }

  // Edit a copy and write it back only when something changed
  LsSceneNodeProperties properties = node->GetProperties();
  bool changed = false;

  char name[128];
  snprintf(name, sizeof(name), "%s", properties.name.c_str());
  if (ImGui::InputText("Name", name, sizeof(name))) {
    properties.name = name;
    changed = true;
  }
  changed |= ImGui::Checkbox("Visible", &properties.visible);

  ImGui::SeparatorText("Placement");
  LsPlacement& placement = properties.placement;
  changed |= ImGui::DragFloat3("Position", &placement.translation.x, 0.01f, 0.0f, 0.0f, "%.3f");
  changed |= ImGui::DragFloat3("Rotation", &placement.rotation.x, 0.5f, 0.0f, 0.0f, "%.1f");
  changed |= ImGui::DragFloat3("Scale", &placement.scale.x, 0.01f, 0.0f, 0.0f, "%.3f");

  if (properties.type == LsNodeType::Camera) {
    ImGui::SeparatorText("Camera");
    changed |= ImGui::DragFloat("Field of view", &properties.lens.verticalFov, 0.1f, 1.0f, 170.0f, "%.1f°", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Vertical, the canvas aspect ratio sets the horizontal one");
    bool active = scene->GetActiveCamera() == selected;
    ImGui::BeginDisabled(active);
    if (ImGui::Checkbox("Paints the canvas", &active)) {
      scene->SetActiveCamera(selected);
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip("The canvas is painted from one camera, check another camera to switch");
  }

  if (properties.type == LsNodeType::Shape) {
    ImGui::SeparatorText("Shape");
    LsShape& shape = properties.shape;
    if (ImGui::BeginCombo("Type", LsShapeTypeName(shape.type))) {
      for (uint32_t i = 0; i < static_cast<uint32_t>(LsShapeType::Count); i++) {
        LsShapeType type = static_cast<LsShapeType>(i);
        if (ImGui::Selectable(LsShapeTypeName(type), type == shape.type) && type != shape.type) {
          shape = LsShape::Default(type);
          changed = true;
        }
      }
      ImGui::EndCombo();
    }

    switch (shape.type) {
      case LsShapeType::Sphere:
        changed |= ImGui::DragFloat("Radius", &shape.params.x, 0.01f, 0.0f, FLT_MAX, "%.3f");
        break;
      case LsShapeType::Box:
        changed |= ImGui::DragFloat3("Half extents", &shape.params.x, 0.01f, 0.0f, FLT_MAX, "%.3f");
        changed |= ImGui::DragFloat("Rounding", &shape.params.w, 0.005f, 0.0f, FLT_MAX, "%.3f");
        shape.params.w = std::min(shape.params.w, std::min(shape.params.x, std::min(shape.params.y, shape.params.z)));
        break;
      case LsShapeType::Capsule:
      case LsShapeType::Cylinder:
        changed |= ImGui::DragFloat("Half height", &shape.params.x, 0.01f, 0.0f, FLT_MAX, "%.3f");
        changed |= ImGui::DragFloat("Radius", &shape.params.y, 0.01f, 0.0f, FLT_MAX, "%.3f");
        break;
      case LsShapeType::Torus:
        changed |= ImGui::DragFloat("Major radius", &shape.params.x, 0.01f, 0.0f, FLT_MAX, "%.3f");
        changed |= ImGui::DragFloat("Minor radius", &shape.params.y, 0.01f, 0.0f, FLT_MAX, "%.3f");
        break;
      case LsShapeType::Plane:
        ImGui::TextDisabled("Infinite plane through the origin, facing +Y");
        break;
      default:
        break;
    }

    ImGui::SeparatorText("Roles");
    changed |= ImGui::CheckboxFlags("Construction", &properties.roles, eRoleConstruction);
    changed |= ImGui::CheckboxFlags("Shadow caster", &properties.roles, eRoleShadowCaster);
    changed |= ImGui::CheckboxFlags("Light emitter", &properties.roles, eRoleLightEmitter);
    changed |= ImGui::CheckboxFlags("Participating media", &properties.roles, eRoleParticipatingMedia);

    if (properties.roles & eRoleLightEmitter) {
      ImGui::SeparatorText("Emission");
      changed |= ImGui::ColorEdit3("Color##Emission", &properties.emissionColor.x, ImGuiColorEditFlags_Float);
      changed |= ImGui::DragFloat("Intensity", &properties.emissionIntensity, 0.05f, 0.0f, FLT_MAX, "%.2f");
    }
    if (properties.roles & eRoleParticipatingMedia) {
      ImGui::SeparatorText("Medium");
      changed |= ImGui::ColorEdit3("Albedo", &properties.mediumAlbedo.x, ImGuiColorEditFlags_Float);
      changed |= ImGui::DragFloat("Density", &properties.mediumDensity, 0.01f, 0.0f, FLT_MAX, "%.3f");
    }
  }

  if (changed) {
    scene->SetProperties(selected, properties);
  }
}
