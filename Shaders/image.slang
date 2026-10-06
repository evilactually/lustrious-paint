struct ImagePushConstants
{
  float2 positions[6]; // KISSing it, all transformations are done on CPU.
};

[[vk::push_constant]]
ConstantBuffer<ImagePushConstants> imagePushConsts;

[[vk::binding(0)]]
Sampler2D samplerColor;

static const float2 inUV[6] = {
  float2(0.0, 0.0),
  float2(1.0, 0.0),
  float2(1.0, 1.0),
  float2(0.0, 0.0),
  float2(1.0, 1.0),
  float2(0.0, 1.0)
};

struct VertexOutput
{
  float4 position : SV_Position;
  [[vk::location(0)]] float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexIndex : SV_VulkanVertexID)
{
  VertexOutput output;
  output.uv = inUV[vertexIndex];
  output.position = float4(imagePushConsts.positions[vertexIndex], 0.0, 1.0);
  return output;
}

[shader("fragment")]
float4 fragmentMain([[vk::location(0)]] float2 uv : TEXCOORD0) : SV_Target0
{
  return samplerColor.Sample(uv);
}
