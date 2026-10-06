struct PaintPushConstants
{
  float2 position;
  float radius;
};

[[vk::push_constant]]
ConstantBuffer<PaintPushConstants> paintPushConstants;

[[vk::binding(0)]]
[format("rgba8")]
RWTexture2D<float4> resultImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 globalId : SV_DispatchThreadID)
{
  float2 delta = float2(globalId.xy) - paintPushConstants.position;
  float r = paintPushConstants.radius;

  if (dot(delta, delta) < r * r) {
    resultImage[globalId.xy] = float4(1.0, 1.0, 1.0, 1.0);
  }
}
