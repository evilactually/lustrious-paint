#pragma once

#include <glm/glm.hpp>
#include <imgui.h>
#include <LsCamera.h>

inline ImVec2 LsToImVec2(glm::vec2 v) { return ImVec2(v.x, v.y); }
inline glm::vec2 LsToVec2(ImVec2 v) { return glm::vec2(v.x, v.y); }

//-------------------------------------------------------------------------------
// @ LsViewportProjection
//-------------------------------------------------------------------------------
// Maps between world space and the screen rectangle a 3D view is drawn in,
// for overlays and mouse interaction. Screen coordinates are ImGui's.
//-------------------------------------------------------------------------------
struct LsViewportProjection
{
  LsCamera camera;
  glm::vec2 origin = glm::vec2(0.0f); // top left of the view
  glm::vec2 size = glm::vec2(1.0f);

  glm::vec2 NdcToScreen(glm::vec2 ndc) const {
    return origin + glm::vec2(ndc.x*0.5f + 0.5f, 0.5f - ndc.y*0.5f)*size;
  }

  glm::vec2 ScreenToNdc(glm::vec2 screen) const {
    glm::vec2 uv = (screen - origin)/size;
    return glm::vec2(uv.x*2.0f - 1.0f, 1.0f - uv.y*2.0f);
  }

  // False for points at or behind the camera
  bool ToScreen(glm::vec3 world, glm::vec2& screen) const {
    glm::vec2 ndc;
    if (!camera.Project(world, ndc)) {
      return false;
    }
    screen = NdcToScreen(ndc);
    return true;
  }

  // Projects a segment, cut where it passes behind the camera. False when all of it is.
  bool SegmentToScreen(glm::vec3 a, glm::vec3 b, glm::vec2& screenA, glm::vec2& screenB) const {
    const float nearDepth = 1.0e-3f;
    float depthA = camera.GetDepth(a);
    float depthB = camera.GetDepth(b);
    if (depthA < nearDepth && depthB < nearDepth) {
      return false;
    }
    if (depthA < nearDepth) {
      a = glm::mix(a, b, (nearDepth - depthA)/(depthB - depthA));
    }
    else if (depthB < nearDepth) {
      b = glm::mix(b, a, (nearDepth - depthB)/(depthA - depthB));
    }
    return ToScreen(a, screenA) && ToScreen(b, screenB);
  }

  void DrawLine(ImDrawList* drawList, glm::vec3 a, glm::vec3 b, ImU32 color, float thickness = 1.0f) const {
    glm::vec2 screenA, screenB;
    if (SegmentToScreen(a, b, screenA, screenB)) {
      drawList->AddLine(LsToImVec2(screenA), LsToImVec2(screenB), color, thickness);
    }
  }

  LsRay GetRay(glm::vec2 screen) const {
    return camera.GenerateRay(ScreenToNdc(screen));
  }

  // World size of a pixel at the depth of the given point
  float GetPixelSize(glm::vec3 world) const {
    float depth = glm::max(camera.GetDepth(world), 1.0e-4f);
    return depth*2.0f*camera.GetTanHalfFovY()/size.y;
  }
};
