#version 450

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
};

layout(push_constant) uniform PushConsts2 {
  vec2 position;
  vec4 colors;
  float size;
} pointPushConsts;

void main() {
  gl_Position = vec4( pointPushConsts.position, 0.0, 1.0 );
  gl_PointSize = pointPushConsts.size;
}
