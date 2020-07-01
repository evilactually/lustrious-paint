#pragma once

#include <cstdint>
#include <LsRenderer.h>
#include <LsVulkanCommands.h>

class LsRenderer;

class LsImage {
	friend LsRenderer;
public:
	~LsImage();
private:
	LsImage(uint32_t width, uint32_t height, LsRenderer const& renderer);
	LsRenderer const& renderer;
	VkImage image;
	VkDeviceMemory memory;
	VkDeviceSize size;
    uint32_t width, height;
};