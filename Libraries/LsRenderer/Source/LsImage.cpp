
#include <LsRenderer.h>
#include <LsImage.h>
#include <LsVulkanCommon.h>

LsImage::LsImage(uint32_t width, uint32_t height, LsRenderer const& renderer) :renderer(renderer)
{
    this->width = width;
    this->height = height;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.extent = { width, height, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkMemoryRequirements memoryRequirements;
    if (vkCreateImage(renderer.device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::string("Could not create image!");
    }

    vkGetImageMemoryRequirements(renderer.device, image, &memoryRequirements);
    // memory requirements give required flags only, not exact type index, we have to find that

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(renderer.physicalDevice, &memoryProperties);
    int32_t memoryType = FindMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VkMemoryAllocateInfo memoryAllocationInfo = {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.allocationSize = memoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryType;
  
    vkAllocateMemory(renderer.device, &memoryAllocationInfo, nullptr, &memory);
    vkBindImageMemory(renderer.device, image, memory, 0);
    // frame memory size used later for writing to file, and later it doubles as frame pitch for reading
    size = memoryRequirements.size;
}

LsImage::~LsImage()
{
    if(memory)
        vkFreeMemory(renderer.device, memory, nullptr);
    if(image)
        vkDestroyImage(renderer.device, image, nullptr);
}

