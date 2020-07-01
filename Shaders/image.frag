#version 450

layout (binding = 0) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  //outFragColor = vec4(inUV, 0.0f, 1.0f);//texture(samplerColor, inUV);
  outFragColor = texture(samplerColor, inUV);
}