#version 450

out gl_PerVertex
{
  vec4 gl_Position;
};

layout (location = 0) out vec2 outUV;

layout(push_constant) uniform PushConsts {
  vec2 positions[6]; // KISSing it, all transformations are done on CPU.
} imagePushConsts;

vec2 inUV[6] = vec2[](
  vec2(0.0f, 0.0f),
  vec2(1.0f, 0.0f),
  vec2(1.0f, 1.0f),
  vec2(0.0f, 0.0f),
  vec2(1.0f, 1.0f),
  vec2(0.0f, 1.0f)
);

void main() {
  outUV = inUV[gl_VertexIndex];
  vec2 positions[6] = imagePushConsts.positions;
  gl_Position = vec4( positions[gl_VertexIndex], 0.0, 1.0 );
}
