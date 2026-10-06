struct PointPushConstants
{
  float2 position;
  float4 color;
  float size;
};

[[vk::push_constant]]
ConstantBuffer<PointPushConstants> pointPushConsts;

struct VertexOutput
{
  float4 position : SV_Position;
  float pointSize : SV_PointSize;
};

[shader("vertex")]
VertexOutput vertexMain()
{
  VertexOutput output;
  output.position = float4(pointPushConsts.position, 0.0, 1.0);
  output.pointSize = pointPushConsts.size;
  return output;
}

[shader("fragment")]
float4 fragmentMain() : SV_Target0
{
  return pointPushConsts.color;
}
