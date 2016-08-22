#version 450

layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform PushConsts {
  vec2 position;
  vec4 color;
  //float size;
} pointPushConsts;

void main() {
  out_Color = pointPushConsts.color;
}
