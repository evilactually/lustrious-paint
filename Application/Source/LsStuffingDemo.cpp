#define GLM_SWIZZLE
#include <LsStuffingDemo.h>
#include <LsWin32MainWindow.h>
#include <glm/glm.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <LsBCCLattice.h>
#include <LsBCCLatticeTypes.h>
#include <LsMath.h>

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
  //lattice = std::make_shared<LsBCCLattice>(std::tuple<int, int, int>(0, 0, 0), std::tuple<int, int, int>(8, 8, 8), 0.5f);
  domain = LsDomain(0.0f, 0.0f, 0.0f, 4.0f, 4.0f, 4.0f);
  lattice = std::make_shared<LsBCCLattice>(domain, 2.0f);
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

  // Pan transformation
  glm::mat4x4 pan_xform = glm::translate(glm::tmat4x4<float>(1.0f), pan);

  // Construct a matrix to place an object in view
  glm::mat4x4 m = makeTSR( rotation, scale, offset )*pan_xform;

  glm::mat4x4 m_no_pan = makeTSR(rotation, scale, offset);
  
  // Construct post-projection transformation
  glm::mat4x4 m2 = glm::translate(glm::tmat4x4<float>(1.0f), { renderer->GetSurfaceWidth() / 2.0f, renderer->GetSurfaceHeight() / 2.0f, 0.0f });
  m2 = glm::scale(m2, glm::vec3(1024.0f, -1024.0f, 1.0f));
  
  // Transform all points
  for (int i = 0; i < 4; i++)
  {
    points[i] = m2*project(m_no_pan*points[i]);
  }
  
  renderer->SetLineWidth(1.0f);
  renderer->SetColor(1.0f, 0.0f, 0.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[1][0], points[1][1]);
  renderer->SetColor(0.0f, 1.0f, 0.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[2][0], points[2][1]);
  renderer->SetColor(0.0f, 0.0f, 1.0f);
  renderer->DrawLine(points[0][0], points[0][1], points[3][0], points[3][1]);
  
  // Draw lattice edges
  renderer->SetLineWidth(1.0f);
  renderer->SetColor(1.0f, 1.0f, 1.0f);
  LsBCCLattice::EdgeIterator eit = lattice->GetEdgeIterator();
  do
  {
    LsBCCEdge edge = eit;
    LsBCCNode n1 = std::get<0>(edge);
    LsBCCNode n2 = std::get<1>(edge);
    glm::vec4 p1 = glm::vec4(lattice->GetNodePosition(n1), 1.0f);
    glm::vec4 p2 = glm::vec4(lattice->GetNodePosition(n2), 1.0f);
    p1 = m2*project(m*p1);
    p2 = m2*project(m*p2);
    if (lattice->GetEdgeColor(edge) == LsBCCColor::eRed) {
      renderer->SetColor(1.0f, 0.0f, 0.0f);
    }
    else {
      renderer->SetColor(1.0f, 1.0f, 0.0f);
    }
    renderer->DrawLine(p1[0], p1[1], p2[0], p2[1]);
  } while (eit.Next());

  // Draw lattice nodes
  renderer->SetPointSize(5.0f);
  LsBCCLattice::NodeIterator nit = lattice->GetNodeIterator();
  do
  {
    LsBCCNode node = nit;
    glm::vec4 position(lattice->GetNodePosition(node), 1.0f);
    position = m2*project(m*position);
    if (lattice->GetNodeColor(node) == LsBCCColor::eRed) {
      renderer->SetColor(1.0f, 0.0f, 0.0f);
    }
    else {
      renderer->SetColor(1.0f, 1.0f, 1.0f);
    }
    renderer->DrawPoint(position[0], position[1]);
  } while (nit.Next());

  // Draw tetrahedra centers
  renderer->SetPointSize(5.0f);
  renderer->SetColor(0.0f, 1.0f, 0.0f);
  renderer->SetLineWidth(2.0f);
  //srand(1);
  LsBCCLattice::TetrahedronIterator tit = lattice->GetTetrahedronIterator();
  do
  {
    LsBCCTetrahedron tetrahedron = tit; // FIXME: This fails when there are no tetrahedra to iterate, need to check a call to Next. Can't. It's a bug.
    glm::vec3 p1 = lattice->GetNodePosition(std::get<0>(tetrahedron));
    glm::vec3 p2 = lattice->GetNodePosition(std::get<1>(tetrahedron));
    glm::vec3 p3 = lattice->GetNodePosition(std::get<2>(tetrahedron));
    glm::vec3 p4 = lattice->GetNodePosition(std::get<3>(tetrahedron));
    glm::vec4 avg = glm::vec4((p1 + p2 + p3 + p4) / 4.0f, 1.0f);
    
    float red = sin(avg.x / 4.0f);
    float green = sin(avg.y / 4.0f);
    float blue = sin(avg.z/4.0f);
    
    avg = m2*project(m*avg);
    
    //float red = LsRandom();
    //float green = LsRandom();
    //float blue = LsRandom();
    
    renderer->SetColor(red, green, blue);
    renderer->DrawPoint(avg[0], avg[1]);
    
    glm::vec4 hp1 = m2*project(m*glm::vec4(p1, 1.0f));
    glm::vec4 hp2 = m2*project(m*glm::vec4(p2, 1.0f));
    glm::vec4 hp3 = m2*project(m*glm::vec4(p3, 1.0f));
    glm::vec4 hp4 = m2*project(m*glm::vec4(p4, 1.0f));
    
    renderer->DrawLine(hp1.xy, hp2.xy);
    renderer->DrawLine(hp1.xy, hp3.xy);
    renderer->DrawLine(hp1.xy, hp4.xy);
    renderer->DrawLine(hp2.xy, hp3.xy);
    renderer->DrawLine(hp3.xy, hp4.xy);
    renderer->DrawLine(hp2.xy, hp4.xy);
  } while (tit.Next());
  
  // Draw domain
  renderer->SetLineWidth(2.0f);
  renderer->SetColor(1.0f, 1.0f, 1.0f);
  glm::vec4 p1 = glm::vec4(domain.x1, domain.y1, domain.z1, 1.0f);
  glm::vec4 p2 = glm::vec4(domain.x2, domain.y1, domain.z1, 1.0f);
  glm::vec4 p3 = glm::vec4(domain.x1, domain.y2, domain.z1, 1.0f);
  glm::vec4 p4 = glm::vec4(domain.x1, domain.y1, domain.z2, 1.0f);
  glm::vec4 p5 = glm::vec4(domain.x1, domain.y2, domain.z2, 1.0f);
  glm::vec4 p6 = glm::vec4(domain.x2, domain.y2, domain.z1, 1.0f);
  glm::vec4 p7 = glm::vec4(domain.x2, domain.y1, domain.z2, 1.0f);
  glm::vec4 p8 = glm::vec4(domain.x2, domain.y2, domain.z2, 1.0f);
  p1 = m2*project(m*p1);
  p2 = m2*project(m*p2);
  p3 = m2*project(m*p3);
  p4 = m2*project(m*p4);
  p5 = m2*project(m*p5);
  p6 = m2*project(m*p6);
  p7 = m2*project(m*p7);
  p8 = m2*project(m*p8);
  renderer->DrawLine(p1[0], p1[1], p2[0], p2[1]);
  renderer->DrawLine(p1[0], p1[1], p3[0], p3[1]);
  renderer->DrawLine(p1[0], p1[1], p4[0], p4[1]);
  renderer->DrawLine(p8[0], p8[1], p5[0], p5[1]);
  renderer->DrawLine(p8[0], p8[1], p6[0], p6[1]);
  renderer->DrawLine(p8[0], p8[1], p7[0], p7[1]);
  
  renderer->DrawLine(p6[0], p6[1], p2[0], p2[1]);
  renderer->DrawLine(p2[0], p2[1], p7[0], p7[1]);
  renderer->DrawLine(p7[0], p7[1], p4[0], p4[1]);
  renderer->DrawLine(p6[0], p6[1], p3[0], p3[1]);
  renderer->DrawLine(p3[0], p3[1], p5[0], p5[1]);
  renderer->DrawLine(p4[0], p4[1], p5[0], p5[1]);

  // Draw pan axis in world space
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
  case WM_MOUSEWHEEL: {
    short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    offset += glm::vec3(0.0f, 0.0f, -wheelDelta / 100.0f);
    }
    break;
  case WM_KEYDOWN:
    domain.x2 += 0.1;// = LsDomain(0.0f, 0.0f, 0.0f, 4.1f, 6.9f, 4.1f);
    domain.x1 += 0.1;
    lattice = std::make_shared<LsBCCLattice>(domain, 2.0f);
    break;
  }
}

