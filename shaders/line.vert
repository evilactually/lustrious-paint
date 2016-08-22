#version 450

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(push_constant) uniform PushConsts {
  vec2 positions[2];
  vec4 colors;
  //float size;
  //float size2;
} linePushConsts;

void main() {
  vec2 positions[2] = linePushConsts.positions;
  gl_Position = vec4( positions[gl_VertexIndex], 0.0, 1.0 );
}
