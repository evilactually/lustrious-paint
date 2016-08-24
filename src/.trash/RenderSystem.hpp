/*
===============================================================================

  RenderSystem is a simplified OpenGL-like layer on top of raw Vulkan

===============================================================================
*/

class RenderSystem
{
public:
  RenderSystem();
  ~RenderSystem();
  // attach data buffer for compute shaders
  // attach vertex buffer for drawing commands
  // attach index buffer
  // attach attributes and bind them to shaders, I don't know yet
  // draw triangles 
  // draw lines
  // attach vertex shader
  // attach pixel shader
  // ...

};

/* 

load some data
create buffer out of it
attach buffer as shader input
attach image as shader input
attach shader
draw primitives

load some data
create buffer out of it
attach buffer as shader input
attach compute shader
dispatch compute

/*