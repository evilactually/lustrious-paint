#define GLM_SWIZZLE
#include <LsStuffingDemo.h>
#include <LsWin32MainWindow.h>
#include <glm/glm.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <LsBCCLattice.h>
#include <LsBCCLatticeTypes.h>

glm::mat4x4 makeTSR(glm::vec3 rotation, float scale, glm::vec3 offset) {
  glm::mat4x4 m = glm::tmat4x4<float>(1.0f);
  m = glm::translate(glm::tmat4x4<float>(1.0f), offset);
  m = glm::scale(m, glm::vec3(scale, scale, scale));
  m = glm::rotate(m, rotation[2], glm::vec3(1.0f, 0.0f, 0.0f));
  m = glm::rotate(m, rotation[1], glm::vec3(0.0f, 1.0f, 0.0f));
  m = glm::rotate(m, rotation[0], glm::vec3(0.0f, 0.0f, 1.0f));
  return m;
}

glm::vec4 project(glm::vec4 point) {
  return glm::vec4(point[0] / point[2], point[1] / point[2], point[2], point[3]);
}

LsStuffingDemo::LsStuffingDemo(std::shared_ptr<LsRenderer> renderer): renderer(renderer) {
  lattice = std::make_shared<LsBCCLattice>(std::tuple<int, int, int>(0, 0, 0), std::tuple<int, int, int>(16, 16, 16), 0.5f);
}

LsStuffingDemo::~LsStuffingDemo()
{
}

void LsStuffingDemo::Render()
{
  glm::vec4 points[4] = { { 0.0f, 0.0f, 0.0f, 1.0f },
                          { 1.0f, 0.0f, 0.0f, 1.0f },
                          { 0.0f, 1.0f, 0.0f, 1.0f },
                          { 0.0f, 0.0f, 1.0f, 1.0f } };
  // Construct a matrix to place an object in view
  glm::mat4x4 m = makeTSR( rotation, scale, offset );
  
  // Construct post-projection transformation
  glm::mat4x4 m2 = glm::translate(glm::tmat4x4<float>(1.0f), { renderer->GetSurfaceWidth() / 2.0f, renderer->GetSurfaceHeight() / 2.0f, 0.0f });
  m2 = glm::scale(m2, glm::vec3(1024.0f, -1024.0f, 1024.0f));
  
  // Transform all points
  for (int i = 0; i < 4; i++)
  {
    points[i] = m2*project(m*points[i]);
  }
  
  renderer->SetLineWidth(1.0f);
  renderer->SetColor(1.0f, 0.0f, 0.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[1][0], points[1][1]);
  renderer->SetColor(0.0f, 1.0f, 0.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[2][0], points[2][1]);
  renderer->SetColor(0.0f, 0.0f, 1.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[3][0], points[3][1]);
    
  // Draw lattice nodes
  renderer->SetPointSize(5.0f);
  LsBCCLattice::NodeIterator it = lattice->GetNodeIterator();
  do
  {
    LsBCCNode node = it;
    glm::vec4 position(lattice->GetNodePosition(node), 1.0f);
    glm::mat4x4 pan_xform = glm::translate(glm::tmat4x4<float>(1.0f), pan);
    position = pan_xform*position;
    position = m2*project(m*position);
    if (lattice->GetNodeColor(node) == LsBCCColor::eRed) {
      renderer->SetColor(1.0f, 0.0f, 0.0f);
    }
    else {
      renderer->SetColor(1.0f, 1.0f, 1.0f);
    }
    renderer->DrawPoint(position[0], position[1]);
  } while (it.Next());

  glm::vec4 points2[3] = { { 0.0f, 0.0f, 0.0f, 1.0f },
                           { 1.0f, 0.0f, 0.0f, 1.0f },
                           { 0.0f, 1.0f, 0.0f, 1.0f } };

  glm::mat4x4 r = glm::tmat4x4<float>(1.0f);
  r = glm::rotate(r, rotation[2], glm::vec3(1.0f, 0.0f, 0.0f));
  r = glm::rotate(r, rotation[1], glm::vec3(0.0f, 1.0f, 0.0f));
  r = glm::rotate(r, rotation[0], glm::vec3(0.0f, 0.0f, 1.0f));
  r = glm::transpose(r);

  for (int i = 0; i < 3; i++)
  {
    points2[i] = m2*project(m*r*points2[i]);
  }

  renderer->SetLineWidth(5.0f);
  renderer->SetColor(1.0f, 0.0f, 0.0f);
  renderer->DrawLine(points2[0][0], points2[0][1], points2[1][0], points2[1][1]);
  renderer->SetColor(0.0f, 1.0f, 0.0f);
  renderer->DrawLine(points2[0][0], points2[0][1], points2[2][0], points2[2][1]);
}

void LsStuffingDemo::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int32_t posx = LOWORD(lParam);
  int32_t posy = HIWORD(lParam);
  float dx = posx_previous - (float)posx;
  float dy = posy_previous - (float)posy;
  switch (uMsg) {
  case WM_MOUSEMOVE:
    if (wParam & MK_LBUTTON)
    {
      rotation += glm::vec3(0.0f, dx*0.01f, dy*0.01f);
    }
    else if (wParam & MK_MBUTTON) {
      // shift along the plane
      // 
      glm::mat4x4 m = glm::tmat4x4<float>(1.0f);
      m = glm::rotate(m, rotation[2], glm::vec3(1.0f, 0.0f, 0.0f));
      m = glm::rotate(m, rotation[1], glm::vec3(0.0f, 1.0f, 0.0f));
      m = glm::rotate(m, rotation[0], glm::vec3(0.0f, 0.0f, 1.0f));
      m = glm::transpose(m);
      glm::vec3 x_pan = (m*glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)).xyz();
      glm::vec3 y_pan = (m*glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)).xyz();
      pan += -(0.01f*dx)*x_pan;
      pan += (0.01f*dy)*y_pan;
      //pan += glm::vec3(dx*0.01f, dy*0.01f, 0.0f);
    }
    posx_previous = (float)posx;
    posy_previous = (float)posy;
    break;
  case WM_MOUSEWHEEL:
    short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    offset += glm::vec3(0.0f, 0.0f, -wheelDelta/100.0f);
    break;
  }
}

