// Lines drawn into the 3D view after the scene has been traced, such as the
// ground grid. Like line.slang one draw is one segment from push constants,
// but the ends are in clip space, so lines are perspective correct and clipped
// at the near plane, and each fragment is hidden where the traced scene is
// nearer. Push constants must match LsSceneLinePushConstants in LsSceneView.cpp.

struct SceneLinePushConstants
{
  float4 positions[2]; // clip space, w is the view depth
  float4 color;        // straight alpha
  float fadeDistance;  // lines fade out towards this view depth, 0 never fades
  float pad0;
  float pad1;
  float pad2;
};

[[vk::push_constant]]
ConstantBuffer<SceneLinePushConstants> sceneLine;

// View depth of the traced scene per pixel, written by viewport.slang. Read only,
// the fragment stage can't have writable storage images without an extra feature.
[[vk::binding(0)]]
Texture2D<float> sceneDepth;

struct VertexOutput
{
  float4 position : SV_Position;
  // Interpolated perspective correctly, so it is the view depth along the line
  [[vk::location(0)]] float depth : DEPTH0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexIndex : SV_VulkanVertexID)
{
  VertexOutput output;
  output.position = sceneLine.positions[vertexIndex];
  output.depth = output.position.w;
  return output;
}

[shader("fragment")]
float4 fragmentMain(float4 fragCoord : SV_Position, [[vk::location(0)]] float depth : DEPTH0) : SV_Target0
{
  float surface = sceneDepth.Load(int3(int2(fragCoord.xy), 0));
  // Lines lying on a surface, the grid on a ground plane, stay on top of it
  if (depth > surface*1.002 + 1.0e-3) {
    discard;
  }
  float4 color = sceneLine.color;
  if (sceneLine.fadeDistance > 0.0) {
    color.a *= saturate(1.0 - depth/sceneLine.fadeDistance);
  }
  return color;
}
