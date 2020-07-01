#pragma once

struct LinePushConstants {
  float positions[4];
  float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct PointPushConstants {
  float positions[2]; 
  float pad[2];
  float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  float size = 1.0f;
};

struct PaintPushConstants {
  float position[2]; // 8
  //float color[4]; // 16
  float radius;
};

struct ImagePushConstants {
	float positions[12]; // 2x4=8
};
