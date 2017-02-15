#include <LsStuffingDemo.h>
#include <LsWin32MainWindow.h>
#include <glm/glm.hpp>
#include <iostream>

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


/*
 Track mouse events, make a spin view, add delta x to RX, y to RY. 
 Iterate over edges to draw edges.
 Iterate over nodes to draw points for nodes, etc.
 Stuffed mesh can be draw by iterating over indecies and sampling vertecies from LsTetrahedronMesh class.
 Make simple animations and record them to gifs.
 Need a perspective projection(calculated on host for now, since I'm using LsRenderer to draw).
*/

LsStuffingDemo::LsStuffingDemo(std::shared_ptr<LsRenderer> renderer): renderer(renderer) { }

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

  // Transform all points
  for (int i = 0; i < 4; i++)
  {
    points[i] = project(m*points[i]);
    glm::mat4x4 m2 = glm::translate(glm::tmat4x4<float>(1.0f), { renderer->GetSurfaceWidth() / 2.0f, renderer->GetSurfaceHeight() / 2.0f, 0.0f });
    m2 = glm::scale(m2, glm::vec3(1024.0f, -1024.0f, 1024.0f));
    points[i] = m2*points[i];
  }
  
  renderer->SetColor(1.0f, 0.0f, 0.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[1][0], points[1][1]);
  renderer->SetColor(0.0f, 1.0f, 0.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[2][0], points[2][1]);
  renderer->SetColor(0.0f, 0.0f, 1.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[3][0], points[3][1]);
}

void LsStuffingDemo::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int32_t posx = LOWORD(lParam);
  int32_t posy = HIWORD(lParam);
  switch (uMsg) {
  case WM_MOUSEMOVE:
    if (wParam & MK_LBUTTON)
    {
      float dx = posx_previous - (float)posx;
      float dy = posy_previous - (float)posy;
      rotation += glm::vec3(0.0f, dx*0.01f, dy*0.01f);
    }
    posx_previous = (float)posx;
    posy_previous = (float)posy;
    break;
  }
}

