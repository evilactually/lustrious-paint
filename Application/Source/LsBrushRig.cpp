#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <LsBrushRig.h>
#include <LsWin32MainWindow.h>
#include <LsRenderer.h>
#include <DWINTAB.h>
#define PACKETDATA (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE 0
#include <wintab/PKTDEF.h>
#include <LsMath.h>
#include <LsGlmHelpers.h>
#include <LsWindowsUtils.h>

#define PRESSURE_SENSITIVITY 20.0f

LsBrushRig::LsBrushRig() {
  renderer = LsRenderer::Get();
}

LsBrushRig::~LsBrushRig() {
  
}

void LsBrushRig::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  PACKET pkt;
  POINT cursor;
  switch (uMsg) {
    case WM_MOUSEMOVE:
    cursor = LsWin32MainWindow::Get()->GetMousePosition();
    penStatus.position[0] = cursor.x;
    penStatus.position[1] = cursor.y;
    //std::cout << cursor.x << " " << cursor.y << std::endl;
    break;
    case WT_PACKET:
    if (WTPacket((HCTX)lParam, wParam, &pkt)) 
    {
      //std::cout << "packet" << std::endl;
      AXIS pressureNormal;
      WTInfoA(WTI_DEVICES|0, DVC_NPRESSURE, &pressureNormal);
      penStatus.pressure = static_cast<float>(pkt.pkNormalPressure)/static_cast<float>(pressureNormal.axMax);
      penStatus.pressure *= PRESSURE_SENSITIVITY;
      penStatus.orientation[0] = (pkt.pkOrientation.orAzimuth/10)*(kPI/180.0f);
      penStatus.orientation[1] = (pkt.pkOrientation.orAltitude/10)*(kPI/180.0f);
      penStatus.orientation[1] = std::abs(penStatus.orientation[1]); // eraser on wacom gives negative altitude
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
