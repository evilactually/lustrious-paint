#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <LsBrushRig.h>
#include <LsRenderer.h>
#include <LsMath.h>
#include <LsGlmHelpers.h>

#define PRESSURE_SENSITIVITY 20.0f

LsBrushRig::LsBrushRig(std::shared_ptr<LsRenderer> renderer) :renderer(renderer) {}

LsBrushRig::~LsBrushRig() {

}

// SDL reports pen tilt as two angles (x tilt positive to the right, y tilt positive towards
// the user), while the rig is built around Wintab style azimuth (clockwise from screen up)
// and altitude (angle above the tablet surface, kPI/2 when the pen is upright).
void LsBrushRig::UpdateOrientationFromTilt() {
  const float tx = std::tan(penStatus.tilt[0]*(kPI/180.0f));
  const float ty = std::tan(penStatus.tilt[1]*(kPI/180.0f));

  float azimuth = std::atan2(tx, -ty);
  if (azimuth < 0.0f) {
    azimuth += 2.0f*kPI;
  }

  penStatus.orientation[0] = azimuth;
  penStatus.orientation[1] = std::atan2(1.0f, std::sqrt(tx*tx + ty*ty));
}

void LsBrushRig::OnSdlEvent(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION:
    penStatus.position[0] = event.motion.x;
    penStatus.position[1] = event.motion.y;
    break;
    case SDL_EVENT_PEN_MOTION:
    penStatus.position[0] = event.pmotion.x;
    penStatus.position[1] = event.pmotion.y;
    break;
    case SDL_EVENT_PEN_UP:
    penStatus.pressure = 0.0f;
    break;
    case SDL_EVENT_PEN_AXIS:
    switch (event.paxis.axis) {
      case SDL_PEN_AXIS_PRESSURE:
      penStatus.pressure = event.paxis.value*PRESSURE_SENSITIVITY; // SDL normalizes pressure to 0..1
      break;
      case SDL_PEN_AXIS_XTILT:
      penStatus.tilt[0] = event.paxis.value;
      UpdateOrientationFromTilt();
      break;
      case SDL_PEN_AXIS_YTILT:
      penStatus.tilt[1] = event.paxis.value;
      UpdateOrientationFromTilt();
      break;
      default:
      break;
    }
    break;
  }
}

void LsBrushRig::Render() {
  glm::tmat2x2<float> rot;
  glm::vec2 pos(0.0f, 1.0f);

  glm::tmat2x2<float> extend = { 32.0f, 0.0f, 
                                 0.0f, 32.0f };

  auto azimuth = tmat2x2_rotation(penStatus.orientation[0])*glm::vec2(0.0f, -32.0f*penStatus.pressure);
  azimuth = azimuth;
  
  const float half_width = 32.0f;
  const float half_height = 32.0f;
  float fcursor[2];
  fcursor[0] = penStatus.position[0] - 1.0f;
  fcursor[1] = penStatus.position[1] - 1.0f;
  float fhalf_width = half_width;
  float fhalf_height = half_height;

  float length = 64.0f;
  glm::vec4 axis_x(1.0f, 0.0f, 0.0f, 1.0f);
  glm::vec4 axis_y(0.0f, 1.0f, 0.0f, 1.0f);
  glm::vec4 axis_z(0.0f, 0.0f, 1.0f, 1.0f);

  // apply altitude by rotation around x
  auto rotX = glm::rotate(glm::tmat4x4<float>(1.0f), -(penStatus.orientation[1] - kPI/2.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  axis_x = rotX*axis_x;
  axis_y = rotX*axis_y;
  axis_z = rotX*axis_z;

  // apply azimuth by rotation around z
  auto rotZ = glm::rotate(glm::tmat4x4<float>(1.0f), penStatus.orientation[0] + kPI, glm::vec3(0.0f, 0.0f, 1.0f));
  axis_x = rotZ*axis_x;
  axis_y = rotZ*axis_y;
  axis_z = rotZ*axis_z;

  // extend to length
  axis_x *= length;
  axis_y *= length;
  axis_z *= length;

  // Draw the brush rig
  renderer->SetLineWidth(1.0f);
  renderer->SetColor(1.0f, 0.0f, 0.0f);
  renderer->DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_x[0], fcursor[1] + axis_x[1]);

  renderer->SetColor(0.0f, 1.0f, 0.0f);
  renderer->DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_y[0], fcursor[1] + axis_y[1]);

  renderer->SetColor(0.0f, 0.0f, 1.0f);
  renderer->DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_z[0], fcursor[1] + axis_z[1]);

  axis_z *= penStatus.pressure;
  renderer->SetColor(1.0f, 1.0f, 1.0f);
  renderer->DrawLine(fcursor[0], fcursor[1], fcursor[0] + axis_z[0], fcursor[1] + axis_z[1]);
}
