#version 450

layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform PushConsts {
  vec2 positions[2];
  vec4 color;
} linePushConsts;

void main() {
  out_Color = linePushConsts.color;
}
