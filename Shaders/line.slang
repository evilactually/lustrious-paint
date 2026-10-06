struct LinePushConstants
{
  float2 positions[2];
  float4 color;
};

[[vk::push_constant]]
ConstantBuffer<LinePushConstants> linePushConsts;

[shader("vertex")]
float4 vertexMain(uint vertexIndex : SV_VulkanVertexID) : SV_Position
{
  return float4(linePushConsts.positions[vertexIndex], 0.0, 1.0);
}

[shader("fragment")]
float4 fragmentMain() : SV_Target0
{
  return linePushConsts.color;
}
