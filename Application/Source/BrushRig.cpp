#include <iostream>
#include <LsBrushRig.h>
#include <LsFWin32MessageHandler.h>
#include <LsRenderer.h>

LsBrushRig::LsBrushRig() {

}

LsBrushRig::~LsBrushRig() {
  
}

void LsBrushRig::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  //std::cout << uMsg << std::endl;
  // LsRenderer::Get()->BeginFrame();
  // LsRenderer::Get()->Clear(0,0,0);
  // LsRenderer::Get()->EndFrame();
}

void LsBrushRig::Render() {
  LsRenderer::Get()->BeginFrame();
  LsRenderer::Get()->Clear(0.1f, 0.1f, 0.1f);
  LsRenderer::Get()->EndFrame();
}
