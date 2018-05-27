//===============================================================================
// @ vulkan_dynamic.h
// 
// A wrapper for vulkan.h header from LunarG SDK that uses dynamically loaded
// commands
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------
#define VK_EXPORTED_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION( fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION( fun ) extern PFN_##fun fun;

#define VK_USE_CORE
#define VK_USE_KHR_DISPLAY
#define VK_USE_KHR_SURFACE
#define VK_USE_KHR_WIN32_SURFACE
#define VK_USE_EXT_DEBUG_REPORT
#define VK_USE_KHR_DISPLAY_SWAPCHAIN
#define VK_USE_KHR_SWAPCHAIN
#define VK_USE_KHR_DEBUG_MARKER
#define VK_USE_AMD_DRAW_INDIRECT_COUNT
#define VK_USE_NV_EXTERNAL_MEMORY_WIN32
#define VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES

#include "LsVulkanCommands.inl"

#undef VK_USE_CORE
#undef VK_USE_KHR_DISPLAY
#undef VK_USE_KHR_SURFACE
#undef VK_USE_KHR_WIN32_SURFACE
#undef VK_USE_EXT_DEBUG_REPORT
#undef VK_USE_KHR_DISPLAY_SWAPCHAIN
#undef VK_USE_KHR_SWAPCHAIN
#undef VK_USE_KHR_DEBUG_MARKER
#undef VK_USE_AMD_DRAW_INDIRECT_COUNT
#undef VK_USE_NV_EXTERNAL_MEMORY_WIN32
#undef VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#include <LsGeometry.h>
#include <glm/glm.hpp>
#include <LsMath.h>
#include <vector>

//-------------------------------------------------------------------------------
// @ LineSegmentHalfTest()
//-------------------------------------------------------------------------------
// Returns distance to line from a point. Function follows a right-hand rule: 
// if base of the index finger is considered as point a and tip as point b, then
// thumb points in direction of the line normal. Distance returned will be positive
// if the point is on the side of the normal, and negative otherwise.
//-------------------------------------------------------------------------------
float DistanceToLine(glm::vec2 a, glm::vec2 b, glm::vec2 p) {
  glm::vec2 w = b - a;
  glm::vec2 n = glm::normalize(glm::vec2(w.y, -w.x));
  glm::vec2 v = p - a;
  float distance = glm::dot(n,v);
  return distance;
}

//-------------------------------------------------------------------------------
// @ PolygonInOutTest()
//-------------------------------------------------------------------------------
// Returns 1 if point is inside the polygon, -1 if otside and 0 if point is on one
// of the edges
//-------------------------------------------------------------------------------
int PolygonInOutTest(std::vector<glm::vec2> polygon, glm::vec2 p) {
  for(auto it = polygon.begin(); it != (polygon.end() - 1); it++) {
    glm::vec2 a = *it;
    glm::vec2 b = *(it + 1);
    float d = DistanceToLine(a, b, p);
    if ( d > -kEpsilon ) {
        return -1; // if at least one half-test fails, point must be outside
    } else if ( !(d > kEpsilon ) ) {
        return 0;  // if point is not inside and not outside of the half-space, it must be on the edge
    }
  }
  return 1; // if it got thus far, the point was not outside or on the edge of any of the half-spaces
}//===============================================================================
// @ LsMath.cpp
// 
// Base math macros and functions
// ------------------------------------------------------------------------------
// Copyright (C) 2004 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

//#include "IvTypes.h"
#include <LsMath.h>
#include <math.h>
#include <algorithm>

float IvSqrt( float val )        { return sqrtf( val ); }
float IvInvSqrt( float val )     { return 1.0f/sqrtf( val ); }
float IvAbs( float f )           { return fabsf(f); }

//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------
extern void IvFastSinCos( float a, float& sina, float& cosa );

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ IsZero()
//-------------------------------------------------------------------------------
// Is this floating point value close to zero?
//-------------------------------------------------------------------------------
bool IsZero( float a ) 
{
  return ( fabsf(a) < kEpsilon );

}   // End of IsZero()

//-------------------------------------------------------------------------------
// @ IvAreEqual()
//-------------------------------------------------------------------------------
// Are these floating point values close to equal?
//-------------------------------------------------------------------------------
bool IvAreEqual( float a, float b ) 
{
  return ( ::IsZero(a-b) );

}  // End of IvAreEqual()

//-------------------------------------------------------------------------------
// @ LsAreEqualRelative()
//-------------------------------------------------------------------------------
// Same as IvAreEqual, but using more advanced method.
//-------------------------------------------------------------------------------
bool LsAreEqualRelative(float a, float b) {
  const float absA = std::abs(a);
  const float absB = std::abs(b);
  const float diff = std::abs(a - b);

  if (a == b) { // shortcut, handles infinities
    return true;
  } else if (a == 0 || b == 0 || diff < FLT_MIN) {
    // a or b is zero or both are extremely close to it
    // relative error is less meaningful here
    return diff < (kEpsilon * FLT_MIN);
  } else { // use relative error
    return diff / std::min((absA + absB), FLT_MAX) < kEpsilon;
  }
}

//-------------------------------------------------------------------------------
// @ IvSin()
//-------------------------------------------------------------------------------
// Returns the floating-point sine of the argument
//-------------------------------------------------------------------------------
float IvSin( float a )
{
  return sinf(a);

}  // End of IvSin


//-------------------------------------------------------------------------------
// @ IvCos()
//-------------------------------------------------------------------------------
// Returns the floating-point cosine of the argument
//-------------------------------------------------------------------------------
float IvCos( float a )
{
  return cosf(a);
}  // End of IvCos


//-------------------------------------------------------------------------------
// @ IvTan()
//-------------------------------------------------------------------------------
// Returns the floating-point tangent of the argument
//-------------------------------------------------------------------------------
float IvTan( float a )
{
  return tanf(a);
}  // End of IvTan

//-------------------------------------------------------------------------------
// @ IvSinCos()
//-------------------------------------------------------------------------------
// Returns the floating-point sine and cosine of the argument
//-------------------------------------------------------------------------------
void IvSinCos( float a, float& sina, float& cosa )
{
  sina = sinf(a);
  cosa = cosf(a);
}  // End of IvSinCos

bool LsOdd(int x) {
  return x % 2;
}

bool LsEven(int x) {
  return !(x % 2);
}

//-------------------------------------------------------------------------------
// @ LsOddCount()
//-------------------------------------------------------------------------------
// Returns a count of odd numbers on the closed interval [min, max]
//-------------------------------------------------------------------------------
int LsOddCount(int min, int max) {
  int N = max - min;
  if ( LsEven(min) )
  {
    return std::ceil(N/2.0f);
  } else {
    return std::floor(N/2.0f) + 1;
  }
}

//-------------------------------------------------------------------------------
// @ LsEvenCount()
//-------------------------------------------------------------------------------
// Returns a count of even numbers on the closed interval [min, max]
//-------------------------------------------------------------------------------
int LsEvenCount(int min, int max) {
  int N = max - min;
  if ( LsOdd(min) )
  {
    return std::ceil(N/2.0f);
  } else {
    return std::floor(N/2.0f) + 1;
  }
}

float LsRandom()
{
  return static_cast<float>(rand())/static_cast<float>(RAND_MAX);
}

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------
//===============================================================================
// @ LsVector3.cpp
// 
// 3D vector class
// ------------------------------------------------------------------------------
// Copyright (C) 2004 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsVector3.h>
#include <LsMath.h>

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

LsVector3 LsVector3::xAxis( 1.0f, 0.0f, 0.0f );
LsVector3 LsVector3::yAxis( 0.0f, 1.0f, 0.0f );
LsVector3 LsVector3::zAxis( 0.0f, 0.0f, 1.0f );
LsVector3 LsVector3::origin( 0.0f, 0.0f, 0.0f );

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsVector3::LsVector3()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
LsVector3::LsVector3(const LsVector3& other) : 
    x( other.x ),
    y( other.y ),
    z( other.z )
{

}   // End of LsVector3::LsVector3()


//-------------------------------------------------------------------------------
// @ LsVector3::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
LsVector3&
LsVector3::operator=(const LsVector3& other)
{
    // if same object
    if ( this == &other )
        return *this;
        
    x = other.x;
    y = other.y;
    z = other.z;
    
    return *this;

}   // End of LsVector3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
// IvWriter& 
// operator<<(IvWriter& out, const LsVector3& source)
// {
//     out << '<' << source.x << ',' << source.y << ',' << source.z << '>';
//
//     return out;
//    
// }   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ LsVector3::Length()
//-------------------------------------------------------------------------------
// Vector length
//-------------------------------------------------------------------------------
float 
LsVector3::Length() const
{
    return IvSqrt( x*x + y*y + z*z );

}   // End of LsVector3::Length()


//-------------------------------------------------------------------------------
// @ LsVector3::LengthSquared()
//-------------------------------------------------------------------------------
// Vector length squared (avoids square root)
//-------------------------------------------------------------------------------
float 
LsVector3::LengthSquared() const
{
    return (x*x + y*y + z*z);

}   // End of LsVector3::LengthSquared()


//-------------------------------------------------------------------------------
// @ ::Distance()
//-------------------------------------------------------------------------------
// Point distance
//-------------------------------------------------------------------------------
float 
Distance( const LsVector3& p0, const LsVector3& p1 )
{
    float x = p0.x - p1.x;
    float y = p0.y - p1.y;
    float z = p0.z - p1.z;

    return ::IvSqrt( x*x + y*y + z*z );

}   // End of LsVector3::Length()


//-------------------------------------------------------------------------------
// @ ::DistanceSquared()
//-------------------------------------------------------------------------------
// Point distance
//-------------------------------------------------------------------------------
float 
DistanceSquared( const LsVector3& p0, const LsVector3& p1 )
{
    float x = p0.x - p1.x;
    float y = p0.y - p1.y;
    float z = p0.z - p1.z;

    return ( x*x + y*y + z*z );

}   // End of ::DistanceSquared()


//-------------------------------------------------------------------------------
// @ LsVector3::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
LsVector3::operator==( const LsVector3& other ) const
{
    if ( ::IvAreEqual( other.x, x )
        && ::IvAreEqual( other.y, y )
        && ::IvAreEqual( other.z, z ) )
        return true;

    return false;   
}   // End of LsVector3::operator==()


//-------------------------------------------------------------------------------
// @ LsVector3::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
LsVector3::operator!=( const LsVector3& other ) const
{
    if ( ::IvAreEqual( other.x, x )
        && ::IvAreEqual( other.y, y )
        && ::IvAreEqual( other.z, z ) )
        return false;

    return true;
}   // End of LsVector3::operator!=()


//-------------------------------------------------------------------------------
// @ LsVector3::IsZero()
//-------------------------------------------------------------------------------
// Check for zero vector
//-------------------------------------------------------------------------------
bool 
LsVector3::IsZero() const
{
    return ::IsZero(x*x + y*y + z*z);

}   // End of LsVector3::IsZero()


//-------------------------------------------------------------------------------
// @ LsVector3::IsUnit()
//-------------------------------------------------------------------------------
// Check for unit vector
//-------------------------------------------------------------------------------
bool 
LsVector3::IsUnit() const
{
    return ::IsZero(1.0f - x*x - y*y - z*z);

}   // End of LsVector3::IsUnit()


//-------------------------------------------------------------------------------
// @ LsVector3::Clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
LsVector3::Clean()
{
    if ( ::IsZero( x ) )
        x = 0.0f;
    if ( ::IsZero( y ) )
        y = 0.0f;
    if ( ::IsZero( z ) )
        z = 0.0f;

}   // End of LsVector3::Clean()


//-------------------------------------------------------------------------------
// @ LsVector3::Normalize()
//-------------------------------------------------------------------------------
// Set to unit vector
//-------------------------------------------------------------------------------
void
LsVector3::Normalize()
{
    float lengthsq = x*x + y*y + z*z;

    if ( ::IsZero( lengthsq ) )
    {
        Zero();
    }
    else
    {
        float factor = IvInvSqrt( lengthsq );
        x *= factor;
        y *= factor;
        z *= factor;
    }

}   // End of LsVector3::Normalize()


//-------------------------------------------------------------------------------
// @ LsVector3::operator+()
//-------------------------------------------------------------------------------
// Add vector to self and return
//-------------------------------------------------------------------------------
LsVector3 
LsVector3::operator+( const LsVector3& other ) const
{
    return LsVector3( x + other.x, y + other.y, z + other.z );

}   // End of LsVector3::operator+()


//-------------------------------------------------------------------------------
// @ LsVector3::operator+=()
//-------------------------------------------------------------------------------
// Add vector to self, store in self
//-------------------------------------------------------------------------------
LsVector3& 
operator+=( LsVector3& self, const LsVector3& other )
{
    self.x += other.x;
    self.y += other.y;
    self.z += other.z;

    return self;

}   // End of LsVector3::operator+=()


//-------------------------------------------------------------------------------
// @ LsVector3::operator-()
//-------------------------------------------------------------------------------
// Subtract vector from self and return
//-------------------------------------------------------------------------------
LsVector3 
LsVector3::operator-( const LsVector3& other ) const
{
    return LsVector3( x - other.x, y - other.y, z - other.z );

}   // End of LsVector3::operator-()


//-------------------------------------------------------------------------------
// @ LsVector3::operator-=()
//-------------------------------------------------------------------------------
// Subtract vector from self, store in self
//-------------------------------------------------------------------------------
LsVector3& 
operator-=( LsVector3& self, const LsVector3& other )
{
    self.x -= other.x;
    self.y -= other.y;
    self.z -= other.z;

    return self;

}   // End of LsVector3::operator-=()

//-------------------------------------------------------------------------------
// @ LsVector3::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
LsVector3
LsVector3::operator-() const
{
    return LsVector3(-x, -y, -z);
}    // End of LsVector3::operator-()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
LsVector3   
LsVector3::operator*( float scalar )
{
    return LsVector3( scalar*x, scalar*y, scalar*z );

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
LsVector3   
operator*( float scalar, const LsVector3& vector )
{
    return LsVector3( scalar*vector.x, scalar*vector.y, scalar*vector.z );

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ LsVector3::operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication by self
//-------------------------------------------------------------------------------
LsVector3&
LsVector3::operator*=( float scalar )
{
    x *= scalar;
    y *= scalar;
    z *= scalar;

    return *this;

}   // End of LsVector3::operator*=()


//-------------------------------------------------------------------------------
// @ operator/()
//-------------------------------------------------------------------------------
// Scalar division
//-------------------------------------------------------------------------------
LsVector3   
LsVector3::operator/( float scalar )
{
    return LsVector3( x/scalar, y/scalar, z/scalar );

}   // End of operator/()


//-------------------------------------------------------------------------------
// @ LsVector3::operator/=()
//-------------------------------------------------------------------------------
// Scalar division by self
//-------------------------------------------------------------------------------
LsVector3&
LsVector3::operator/=( float scalar )
{
    x /= scalar;
    y /= scalar;
    z /= scalar;

    return *this;

}   // End of LsVector3::operator/=()


//-------------------------------------------------------------------------------
// @ LsVector3::Dot()
//-------------------------------------------------------------------------------
// Dot product by self
//-------------------------------------------------------------------------------
float               
LsVector3::Dot( const LsVector3& vector ) const
{
    return (x*vector.x + y*vector.y + z*vector.z);

}   // End of LsVector3::Dot()


//-------------------------------------------------------------------------------
// @ Dot()
//-------------------------------------------------------------------------------
// Dot product friend operator
//-------------------------------------------------------------------------------
float               
Dot( const LsVector3& vector1, const LsVector3& vector2 )
{
    return (vector1.x*vector2.x + vector1.y*vector2.y + vector1.z*vector2.z);

}   // End of Dot()


//-------------------------------------------------------------------------------
// @ LsVector3::Cross()
//-------------------------------------------------------------------------------
// Cross product by self
//-------------------------------------------------------------------------------
LsVector3   
LsVector3::Cross( const LsVector3& vector ) const
{
    return LsVector3( y*vector.z - z*vector.y,
                      z*vector.x - x*vector.z,
                      x*vector.y - y*vector.x );

}   // End of LsVector3::Cross()


//-------------------------------------------------------------------------------
// @ Cross()
//-------------------------------------------------------------------------------
// Cross product friend operator
//-------------------------------------------------------------------------------
LsVector3               
Cross( const LsVector3& vector1, const LsVector3& vector2 )
{
    return LsVector3( vector1.y*vector2.z - vector1.z*vector2.y,
                      vector1.z*vector2.x - vector1.x*vector2.z,
                      vector1.x*vector2.y - vector1.y*vector2.x );

}   // End of Cross()


#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <assert.h>
#include <array>
#include <vulkan_dynamic.hpp>

#include <destructor.h>
#include <LsVulkanLoader.h>
#include <LsError.h>
#include <LsVulkanInstance.h>
#include <LsVulkanDevice.h>
#include <LsVulkanPresentation.h>
#include <LsPushConstantTypes.h>
#include <LsVulkanRendering.h>
#include <LsVulkanCommandBuffers.h>
#include <LsRenderer.h>

#ifdef GIF_RECORDING
#include <FreeImage.h>
#endif

using namespace lslib;

//-------------------------------------------------------------------------------
// @ requiredInstanceExtensions
//-------------------------------------------------------------------------------
// List of instance-level extension that will be enabled when creating an instance
//-------------------------------------------------------------------------------
std::vector<const char*> requiredInstanceExtensions = {
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef VULKAN_VALIDATION
  VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
};

//-------------------------------------------------------------------------------
// @ requiredDeviceExtensions
//-------------------------------------------------------------------------------
// List of device-level extension that will be enabled when creating a logical 
// device
//-------------------------------------------------------------------------------
std::vector<const char*> requiredDeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//-------------------------------------------------------------------------------
// @ requiredInstanceValidationLayers
//-------------------------------------------------------------------------------
// List of instance-level validation levels that will be enabled when creating an
// instance
//-------------------------------------------------------------------------------
std::vector<const char*> requiredInstanceValidationLayers = {
#ifdef VULKAN_VALIDATION
  "VK_LAYER_LUNARG_standard_validation"
#endif
};

// Needs device, swapchain, graphics queue

//LsRenderer::LsRenderer() {
//
//}

// LsRenderer LsRenderer::renderer;

//LsRenderer* LsRenderer::Get() {
//  return &renderer;
//}

#ifdef GIF_RECORDING
void LsRenderer::CreateCapturedFrameImage() {
  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.flags = 0;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageCreateInfo.extent = { swapChainInfo.extent.width, swapChainInfo.extent.height, 1 };
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
  if (vkCreateImage(device, &imageCreateInfo, nullptr, &captureInfo.capturedFrameImage.image) != VK_SUCCESS) {
    throw std::string("Could not create image!");
  }
  
  vkGetImageMemoryRequirements(device, captureInfo.capturedFrameImage.image, &memoryRequirements);
  // memory requirements give required flags only, not exact type index, we have to find that

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  int32_t memoryType = FindMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VkMemoryAllocateInfo memoryAllocationInfo = {};
  memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocationInfo.allocationSize = memoryRequirements.size;
  memoryAllocationInfo.memoryTypeIndex = memoryType;
  
  vkAllocateMemory(device, &memoryAllocationInfo, nullptr, &captureInfo.capturedFrameImage.memory);
  vkBindImageMemory(device, captureInfo.capturedFrameImage.image, captureInfo.capturedFrameImage.memory, 0);
  // frame memory size used later for writing to file, and later it doubles as frame pitch for reading
  captureInfo.capturedFrameImage.size = memoryRequirements.size;
}

void LsRenderer::RecordFrameCaptureCmds() {
  VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
  cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  cmd_buffer_begin_info.pInheritanceInfo = nullptr;
  
  vkBeginCommandBuffer( captureInfo.captureFrameCmds, &cmd_buffer_begin_info );
  
  VkImageSubresourceRange imageSubresourceRange;
  imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageSubresourceRange.baseMipLevel = 0;
  imageSubresourceRange.levelCount = 1;
  imageSubresourceRange.baseArrayLayer = 0;
  imageSubresourceRange.layerCount = 1;

  // transition swap chain image into transfer source layout
  CmdImageBarrier(captureInfo.captureFrameCmds, // cmdBuffer
    VK_ACCESS_MEMORY_READ_BIT,                  // srcAccessMask
    VK_ACCESS_TRANSFER_READ_BIT,                // dstAccessMask
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // oldLayout
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,       // newLayout
    VK_QUEUE_FAMILY_IGNORED,                    // srcQueueFamilyIndex
    VK_QUEUE_FAMILY_IGNORED,                    // dstQueueFamilyIndex                             
    swapChainInfo.images[swapChainInfo.acquiredImageIndex], // image
    imageSubresourceRange,                      // subresourceRange
    0,                                          // srcStageMask, wait for nothing
    VK_PIPELINE_STAGE_TRANSFER_BIT,             // dstStageMask, block transfer
    0);                                         // dependencyFlags

  // transition captureInfo.capturedFrameImage.image to transfer destination layout
  CmdImageBarrier(captureInfo.captureFrameCmds,  // cmdBuffer
                  0,                             // srcAccessMask
                  VK_ACCESS_TRANSFER_WRITE_BIT,  // dstAccessMask
                  VK_IMAGE_LAYOUT_UNDEFINED,     // oldLayout
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newLayout
                  VK_QUEUE_FAMILY_IGNORED,
                  VK_QUEUE_FAMILY_IGNORED,
                  captureInfo.capturedFrameImage.image,
                  imageSubresourceRange,
                  0,                              // wait for nothing
                  VK_PIPELINE_STAGE_TRANSFER_BIT, // block transfer
                  0);

  VkImageSubresourceLayers subresourceLayers = {
    VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags                     aspectMask
    0,                         // uint32_t                               mipLevel
    0,                         // uint32_t                               baseArrayLayer
    1                          // uint32_t                               layerCount
  };

  VkImageCopy region = {
    subresourceLayers,
    { 0,0,0 },
    subresourceLayers,
    { 0,0,0 },
    { swapChainInfo.extent.width, swapChainInfo.extent.height, 1 }
  };

  vkCmdCopyImage(captureInfo.captureFrameCmds,
                 swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 captureInfo.capturedFrameImage.image,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1,
                 &region);

  // transition captureInfo.capturedFrameImage.image to host read layout
  CmdImageBarrier(captureInfo.captureFrameCmds,
                  VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_ACCESS_HOST_READ_BIT,        // image memory will be mapped for reading later
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_GENERAL,        // layout that supports host access
                  VK_QUEUE_FAMILY_IGNORED,
                  VK_QUEUE_FAMILY_IGNORED,
                  captureInfo.capturedFrameImage.image,
                  imageSubresourceRange,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, // wait for transfer
                  VK_PIPELINE_STAGE_HOST_BIT,     // guarantee visibility of writes to host
                  0);

  // transition swap chain image back to presentation layout
  CmdImageBarrier(captureInfo.captureFrameCmds,
                  VK_ACCESS_TRANSFER_READ_BIT,
                  VK_ACCESS_MEMORY_READ_BIT,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  VK_QUEUE_FAMILY_IGNORED,
                  VK_QUEUE_FAMILY_IGNORED,
                  swapChainInfo.images[swapChainInfo.acquiredImageIndex],
                  imageSubresourceRange,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, // wait for transfer
                  0,                              // block nothing, because nothing comes next
                  0);
  vkEndCommandBuffer(captureInfo.captureFrameCmds);
}

void LsRenderer::StartGIFRecording(std::string filename) {
  std::cout << "Recording started" << std::endl;
  OpenBufferFile();
  CreateCapturedFrameImage();
  RecordFrameCaptureCmds();
  // frame row pitch is used for decoding frame memory
  VkImageSubresource imageSubresource = {
    VK_IMAGE_ASPECT_COLOR_BIT,
    0,
    0
  };

  VkSubresourceLayout subresourceLayout;
  vkGetImageSubresourceLayout(device, 
                              captureInfo.capturedFrameImage.image,
                              &imageSubresource,
                              &subresourceLayout);

  captureInfo.capturedFrameImage.rowPitch = subresourceLayout.rowPitch;
  // capture frame extent
  captureInfo.capturedFrameImage.extent = swapChainInfo.extent;
  //RecordFrameCaptureCmds();
  captureInfo.capturing = true;
  captureInfo.captureFrameCount = 0;
  captureInfo.startTime = std::chrono::high_resolution_clock::now();
  captureInfo.filename = filename;
}

void LsRenderer::StopGIFRecording() {
  std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
  BYTE* captureData = (BYTE*)malloc(captureInfo.capturedFrameImage.size);
  DWORD bytesRead;
  std::cout << "Recording stopped" << std::endl;
  SetFilePointer(captureInfo.hCaptureBufferFile,// hFile
    0,                                          // lDistanceToMove
    0,                                          // lpDistanceToMoveHigh
    FILE_BEGIN);                                // dwMoveMethod
  char fileName[1024];
  //BOOL dialogResult = ShowSaveAsDialog(fileName, sizeof(fileName), "*.gif", "gif");
  //if (!dialogResult) {
  //  free(captureData);
  //  CloseHandle(captureInfo.hCaptureBufferFile);
  //  captureInfo.capturing = false;
  //  return;
  //}
  std::cout << "Saving to " << captureInfo.filename << std::endl;
  FIMULTIBITMAP* multibitmap = FreeImage_OpenMultiBitmap(
    FIF_GIF,
    captureInfo.filename.c_str(),
    TRUE,
    FALSE);
  if (!multibitmap)
  {
    throw std::string("FreeImage_OpenMultiBitmap failed to create gif file!");
  }
  double captureDuration = std::chrono::duration<double, std::milli>(endTime - captureInfo.startTime).count() / 1000.0;
  double fps = 30.0f;//((double)captureInfo.captureFrameCount)/captureDuration;
  DWORD frameTimeMs = (DWORD)((1000.0 / fps) + 0.5);
  //DWORD frameTimeMs = (DWORD)( ((double)captureInfo.captureFrameCount)/captureDuration + 0.5);
  std::cout << ((double)captureInfo.captureFrameCount)/captureDuration << std::endl;

  FITAG* tag = FreeImage_CreateTag();
  FreeImage_SetTagKey(tag, "FrameTime");
  FreeImage_SetTagType(tag, FIDT_LONG);
  FreeImage_SetTagCount(tag, 1);
  FreeImage_SetTagLength(tag, 4);
  FreeImage_SetTagValue(tag, &frameTimeMs);
  while (captureInfo.captureFrameCount) {
    // read next captured frame
    BOOL readResult = ReadFile(captureInfo.hCaptureBufferFile,
      captureData,
      captureInfo.capturedFrameImage.size,
      &bytesRead,
      nullptr);
    if (!readResult || (bytesRead != captureInfo.capturedFrameImage.size)) {
      std::cout << "ReadFile failed:" << GetLastError() << std::endl;
      throw std::string("ReadFile failed");
    }
    FIBITMAP* bitmap = FreeImage_Allocate(captureInfo.capturedFrameImage.extent.width,
      captureInfo.capturedFrameImage.extent.height,
      24);
    RGBQUAD color;
    for (int y = 0; y < captureInfo.capturedFrameImage.extent.height; ++y)
    {
      for (int x = 0; x < captureInfo.capturedFrameImage.extent.width; ++x)
      {
        BYTE* pixel = captureData + captureInfo.capturedFrameImage.rowPitch*y + 4 * x;
        color.rgbRed = *(pixel + 2);
        color.rgbGreen = *(pixel + 1);
        color.rgbBlue = *pixel;
        FreeImage_SetPixelColor(bitmap, x, captureInfo.capturedFrameImage.extent.height - y, &color);
      }
    }
    FIBITMAP* quantized = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT);
    // Reset animation metadata
    FreeImage_SetMetadata(FIMD_ANIMATION, quantized, NULL, NULL);
    FreeImage_SetMetadata(FIMD_ANIMATION, quantized, FreeImage_GetTagKey(tag), tag);
    //FreeImage_Save(FIF_PNG, bitmap, (std::to_string(captureInfo.captureFrameCount)+std::string(".png")).c_str(), 0);
    FreeImage_AppendPage(multibitmap, quantized);
    if (quantized) {
      FreeImage_Unload(quantized);
    }
    FreeImage_Unload(bitmap);
    captureInfo.captureFrameCount--;
    //captureData += captureInfo.capturedFrameImage.size;
  }
  FreeImage_CloseMultiBitmap(multibitmap);
  free(captureData);
  //UnmapViewOfFile(captureData);
  //CloseHandle(hMapping);
  CloseHandle(captureInfo.hCaptureBufferFile);
  captureInfo.capturing = false;
}

void LsRenderer::OpenBufferFile() {
  TCHAR tmpPath[MAX_PATH];
  TCHAR tmpFilePath[MAX_PATH];
  DWORD result = GetTempPath(MAX_PATH, tmpPath);
  if ((result > MAX_PATH) || (result == 0)) {
    throw std::string("GetTempPath failed!");
  }
  std::cout << "Using temporary path:" << tmpPath << std::endl;
  result = GetTempFileName(tmpPath, TEXT("ls"), 0, tmpFilePath);
  std::cout << "Buffer temporary file" << tmpFilePath << std::endl;
  if (!result) {
    throw std::string("GetTempFileName failed!");
  }

  captureInfo.hCaptureBufferFile = CreateFile(tmpFilePath,
    GENERIC_WRITE | GENERIC_READ,
    0,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
    NULL);
  if (captureInfo.hCaptureBufferFile == INVALID_HANDLE_VALUE) {
    throw std::string("Failed to create a file!");
  }
}

bool LsRenderer::CheckFrameSavingFinished() {
  if (captureInfo.hSavingThread) {
    DWORD result = WaitForSingleObject(captureInfo.hSavingThread, 0); // no wait
    return !result; // it's 0 when finished
  }
  else {
    return true; // no thread was started
  }
}

DWORD WINAPI WriteFrameThread(LPVOID lpParam) {
  LsRenderer* renderer = (LsRenderer*)lpParam;
  void* data;
  DWORD written;
  vkMapMemory(renderer->device, renderer->captureInfo.capturedFrameImage.memory, 0, renderer->captureInfo.capturedFrameImage.size, 0, &data);
  DWORD result = WriteFile(renderer->captureInfo.hCaptureBufferFile, data, renderer->captureInfo.capturedFrameImage.size, &written, nullptr);
  vkUnmapMemory(renderer->device, renderer->captureInfo.capturedFrameImage.memory);
  //TODO: What if write fails?
  return 0;
}

void LsRenderer::BeginSavingCapturedFrame() {
  captureInfo.captureFrameCount++;
  captureInfo.hSavingThread = CreateThread(NULL,
    0,
    WriteFrameThread,
    this, // pass instance of LsRenderer
    0,
    NULL);
}
#endif

LsRenderer::LsRenderer(HINSTANCE hInstance, HWND window):window(window) {
  //LsRenderer* renderer = LsRenderer::Get();
  
#ifdef GIF_RECORDING
  FreeImage_Initialise();
#endif

  LsLoadVulkanLibrary();

  LsLoadExportedEntryPoints();
  LsLoadGlobalLevelEntryPoints();

  CreateInstance(requiredInstanceExtensions, requiredInstanceValidationLayers, &instance);
 
  LsLoadInstanceLevelEntryPoints(instance, requiredInstanceExtensions);

#ifdef VULKAN_VALIDATION
  CreateDebugReportCallback(instance, &debugReportCallback);
#endif
  
  CreatePresentationSurface(instance, hInstance, window, &swapChainInfo.presentationSurface);

  CreateDevice(instance, 
               swapChainInfo.presentationSurface,
               requiredDeviceExtensions,
               &physicalDevice,
               &device,
               &graphicsQueue.familyIndex,
               &presentQueue.familyIndex);

  LsLoadDeviceLevelEntryPoints(device, requiredDeviceExtensions);

  commandPool = CreateCommandPool(device, graphicsQueue.familyIndex);
  commandBuffer = CreateCommandBuffer(device, commandPool);
#ifdef GIF_RECORDING
  captureInfo.captureFrameCmds = CreateCommandBuffer(device, commandPool);
#endif
 
  CreateSemaphore(device, &semaphores.imageAvailable);
  CreateSemaphore(device, &semaphores.renderingFinished);
#ifdef GIF_RECORDING
  CreateSemaphore(device, &captureInfo.captureFinishedSemaphore);
#endif

  shaderModules.lineVertexShader = CreateShaderModule(device, "Shaders/line.vert.spv");
  shaderModules.lineFragmentShader = CreateShaderModule(device, "Shaders/line.frag.spv");
  shaderModules.pointVertexShader = CreateShaderModule(device, "Shaders/point.vert.spv");
  shaderModules.pointFragmentShader = CreateShaderModule(device, "Shaders/point.frag.spv");
  
  CreateFence(device, &submitCompleteFence, true);
#ifdef GIF_RECORDING
  CreateFence(device, &captureInfo.captureCompleteFence, false);
#endif

  vkGetDeviceQueue( device, graphicsQueue.familyIndex, 0, &graphicsQueue.handle );
  vkGetDeviceQueue( device, presentQueue.familyIndex, 0, &presentQueue.handle );

  if ( CreateSwapChain(physicalDevice,
                       device,
                       swapChainInfo.presentationSurface,
                       window,
                       &swapChainInfo.swapChain,
                       &swapChainInfo.format,
                       &swapChainInfo.extent) ) {

    swapChainInfo.images = GetSwapChainImages(device, swapChainInfo.swapChain);

    swapChainInfo.imageViews = CreateSwapChainImageViews(device,
                                                                   swapChainInfo.images,
                                                                   swapChainInfo.format);

    renderPass = CreateSimpleRenderPass(device, swapChainInfo.format);

    framebuffers = CreateFramebuffers(device, 
                                                swapChainInfo.imageViews,
                                                swapChainInfo.extent,
                                                renderPass);

    linePipelineLayout = CreatePipelineLayout(device, sizeof(LinePushConstants));
    pointPipelineLayout = CreatePipelineLayout(device, sizeof(PointPushConstants));

    CreatePrimitivePipelines(device,
                             shaderModules.lineVertexShader,
                             shaderModules.lineFragmentShader,
                             shaderModules.pointVertexShader,
                             shaderModules.pointFragmentShader,
                             linePipelineLayout,
                             pointPipelineLayout,
                             renderPass,
                             swapChainInfo.extent,
                             &linePipeline,
                             &pointPipeline);
    
    canRender = true;
  }
}

void LsRenderer::RefreshSwapChain() {
  canRender = false;

  if ( device ) {
    vkDeviceWaitIdle(device);
  
    if ( linePipeline )
    {
      vkDestroyPipeline( device, linePipeline, nullptr);
	  linePipeline = VK_NULL_HANDLE;
    }

    if ( pointPipeline )
    {
      vkDestroyPipeline(device, pointPipeline, nullptr);
	  pointPipeline = VK_NULL_HANDLE;
    }

    if ( linePipelineLayout ) {
      vkDestroyPipelineLayout(device, linePipelineLayout, nullptr);
	  linePipelineLayout = VK_NULL_HANDLE;
    }

    if ( pointPipelineLayout ) {
      vkDestroyPipelineLayout(device, pointPipelineLayout, nullptr);
	  pointPipelineLayout = VK_NULL_HANDLE;
    }

    for ( const VkFramebuffer& framebuffer:framebuffers ) {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    framebuffers.clear();

    if ( renderPass ) {
      vkDestroyRenderPass(device, renderPass, nullptr);
	  renderPass = VK_NULL_HANDLE;
    }

    for( auto &imageView: swapChainInfo.imageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }
    swapChainInfo.imageViews.clear();

    // We don't have to destroy the swap chain here, it will be 
    // destroyed by a call to CreateSwapChain
  }

  if ( CreateSwapChain(physicalDevice,
                       device,
                       swapChainInfo.presentationSurface,
                       window,
                       &swapChainInfo.swapChain,
                       &swapChainInfo.format,
                       &swapChainInfo.extent) ) {
    
    swapChainInfo.images = GetSwapChainImages(device, swapChainInfo.swapChain);

    swapChainInfo.imageViews = CreateSwapChainImageViews(device,
                                                         swapChainInfo.images,
                                                         swapChainInfo.format);

    renderPass = CreateSimpleRenderPass(device, swapChainInfo.format);

    framebuffers = CreateFramebuffers(device, 
                                      swapChainInfo.imageViews,
                                      swapChainInfo.extent,
                                      renderPass);

    linePipelineLayout = CreatePipelineLayout(device, sizeof(LinePushConstants));
    pointPipelineLayout = CreatePipelineLayout(device, sizeof(PointPushConstants));

    CreatePrimitivePipelines(device,
                             shaderModules.lineVertexShader,
                             shaderModules.lineFragmentShader,
                             shaderModules.pointVertexShader,
                             shaderModules.pointFragmentShader,
                             linePipelineLayout,
                             pointPipelineLayout,
                             renderPass,
                             swapChainInfo.extent,
                             &linePipeline,
                             &pointPipeline);

	windowToVulkanTransformation = WindowToVulkanTransformation(window);

    canRender = true;
  } else {
    std::cout << "Swap chain not ready, cannot render." << std::endl;
  }
}

int LsRenderer::GetSurfaceWidth()
{
  return swapChainInfo.extent.width;
}

int LsRenderer::GetSurfaceHeight()
{
  return swapChainInfo.extent.height;
}

LsRenderer::~LsRenderer() {
  canRender = false;

  if ( device ) {
    vkDeviceWaitIdle(device);
  
    if ( linePipeline )
    {
      vkDestroyPipeline( device, linePipeline, nullptr );
    }

    if ( pointPipeline )
    {
      vkDestroyPipeline( device, pointPipeline, nullptr );
    }

    if ( linePipelineLayout ) {
      vkDestroyPipelineLayout( device, linePipelineLayout, nullptr );
    }

    if ( pointPipelineLayout ) {
	  vkDestroyPipelineLayout( device, pointPipelineLayout, nullptr );
    }

    for ( const VkFramebuffer& framebuffer:framebuffers ) {
      vkDestroyFramebuffer( device, framebuffer, nullptr );
    }

    if ( renderPass )
      vkDestroyRenderPass( device, renderPass, nullptr );

    for( auto &imageView: swapChainInfo.imageViews) {
      vkDestroyImageView( device, imageView, nullptr );
    }

    if ( swapChainInfo.swapChain )
       vkDestroySwapchainKHR( device, swapChainInfo.swapChain, nullptr );

    if ( submitCompleteFence )
      vkDestroyFence( device, submitCompleteFence, nullptr );

    if ( shaderModules.lineVertexShader )
      vkDestroyShaderModule( device, shaderModules.lineVertexShader, nullptr );
    if ( shaderModules.lineFragmentShader )
      vkDestroyShaderModule( device, shaderModules.lineFragmentShader, nullptr );
    if ( shaderModules.pointVertexShader )
      vkDestroyShaderModule( device, shaderModules.pointVertexShader, nullptr );
    if ( shaderModules.pointFragmentShader )
      vkDestroyShaderModule( device, shaderModules.pointFragmentShader, nullptr );

    if ( semaphores.renderingFinished )
      vkDestroySemaphore( device, semaphores.renderingFinished, nullptr );

    if ( semaphores.imageAvailable )
      vkDestroySemaphore( device, semaphores.imageAvailable, nullptr );

    if( commandPool ) {
      vkDestroyCommandPool( device, commandPool, nullptr );
	  commandPool = VK_NULL_HANDLE;
    }

	vkDestroyDevice(device, nullptr);
  }

  if ( swapChainInfo.presentationSurface )
    vkDestroySurfaceKHR( instance, swapChainInfo.presentationSurface, nullptr );

#ifdef VULKAN_VALIDATION
  if ( debugReportCallback )
    vkDestroyDebugReportCallbackEXT( instance, debugReportCallback, nullptr );
#endif // VULKAN_VALIDATION

  if (instance)
    vkDestroyInstance( instance, nullptr );

  LsUnloadVulkanLibrary();
}

void LsRenderer::BeginFrame() {
  if( vkWaitForFences( device, 1, &submitCompleteFence, VK_FALSE, 1000000000 ) != VK_SUCCESS ) {
    throw std::string("Waiting for fence takes too long!");
  }

  vkResetFences( device, 1, &submitCompleteFence );

  VkResult result = vkAcquireNextImageKHR( device,
                                           swapChainInfo.swapChain, 
                                           UINT64_MAX,
                                           semaphores.imageAvailable,
                                           VK_NULL_HANDLE,
                                           &swapChainInfo.acquiredImageIndex );

  switch( result ) {
    case VK_SUCCESS:
    break;
    case VK_SUBOPTIMAL_KHR:
    // It's still OK to use.
    break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    RefreshSwapChain();
    return;
    default:
    throw std::string( "Problem occurred during swap chain image acquisition!" );
  }

  VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
  cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  vkBeginCommandBuffer( commandBuffer, &cmd_buffer_begin_info );

  VkImageSubresourceRange image_subresource_range = {
	VK_IMAGE_ASPECT_COLOR_BIT,                      // VkImageAspectFlags                     aspectMask
	0,                                              // uint32_t                               baseMipLevel
	1,                                              // uint32_t                               levelCount
	0,                                              // uint32_t                               baseArrayLayer
	1                                               // uint32_t                               layerCount
  };

  // Transition to presentation layout and tell vulkan that we are discarding previous contents of the image
  // block reads from present(atachment output), blocks draws and clears
  VkImageMemoryBarrier barrier_from_present_to_draw = {}; 
  barrier_from_present_to_draw.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_from_present_to_draw.srcAccessMask = 0;
  barrier_from_present_to_draw.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier_from_present_to_draw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier_from_present_to_draw.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier_from_present_to_draw.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_draw.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_draw.image = swapChainInfo.images[swapChainInfo.acquiredImageIndex];
  barrier_from_present_to_draw.subresourceRange = image_subresource_range;

  vkCmdPipelineBarrier( commandBuffer, 
	                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_draw );
}

void LsRenderer::EndFrame() {
#ifdef GIF_RECORDING
  BOOL captureInitiated = false;
#endif
  if( drawingContext.drawing ) {
    EndDrawing();
  }

  if( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS ) {
    throw std::string("Could not record command buffers!");
  }

  // stall these stages until image is available from swap chain
  VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
	                                           VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &semaphores.imageAvailable;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &commandBuffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &semaphores.renderingFinished;
  submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
  
  if( vkQueueSubmit( graphicsQueue.handle, 1, &submit_info, submitCompleteFence ) != VK_SUCCESS ) {
    throw std::string("Submit to queue failed!");
  }

  VkSemaphore presentBlockingSemaphore = semaphores.renderingFinished;

#ifdef GIF_RECORDING
  if (captureInfo.capturing) {
    // capture next frame only if previous capture already finished
    if (CheckFrameSavingFinished()) {
      std::cout << captureInfo.captureFrameCount << std::endl;
      // stall transfer stage until rendering is finished 
      wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

      submit_info.waitSemaphoreCount = 1;
      submit_info.pWaitSemaphores = &semaphores.renderingFinished;
      submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &captureInfo.captureFrameCmds;
      submit_info.signalSemaphoreCount = 1;
      submit_info.pSignalSemaphores = &captureInfo.captureFinishedSemaphore;

      vkResetFences( device, 1, &captureInfo.captureCompleteFence );
      if (vkQueueSubmit( graphicsQueue.handle, 1, &submit_info, captureInfo.captureCompleteFence) != VK_SUCCESS ) {
        throw std::string("Submit to queue failed!");
      }
      // make present wait for capture instead of just render
      presentBlockingSemaphore = captureInfo.captureFinishedSemaphore;
      // resume presentation as soon as possible, using this flag to resume capture
      captureInitiated = true;
    }
    else {
      std::cout << "Frame skipped, still writing!" << std::endl;
    }
  }
#endif

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &presentBlockingSemaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapChainInfo.swapChain;
  present_info.pImageIndices = &swapChainInfo.acquiredImageIndex;
  present_info.pResults = nullptr;
  
  VkResult result = vkQueuePresentKHR(presentQueue.handle, &present_info );

  switch( result ) {
    case VK_SUCCESS:
    break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
    RefreshSwapChain();
    return;
    default:
    throw std::string("Problem occurred during image presentation!");
  }

#ifdef GIF_RECORDING
  if (captureInitiated) {
    // wait for capture submission to finish
    if (vkWaitForFences(device, 1, &captureInfo.captureCompleteFence, VK_FALSE, 1000000000) != VK_SUCCESS) {
      throw std::string("Waiting for fence takes too long!");
    }
    // async copy frame to file
    BeginSavingCapturedFrame();
  }
#endif
}

void LsRenderer::Clear(float r, float g, float b) {
  if( drawingContext.drawing ) {
    EndDrawing();
  }

  VkImageSubresourceRange image_subresource_range = {
	VK_IMAGE_ASPECT_COLOR_BIT,                      // VkImageAspectFlags                     aspectMask
	0,                                              // uint32_t                               baseMipLevel
	1,                                              // uint32_t                               levelCount
	0,                                              // uint32_t                               baseArrayLayer
	1                                               // uint32_t                               layerCount
  };

  VkImageMemoryBarrier barrier_from_present_to_clear = {};
  barrier_from_present_to_clear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_from_present_to_clear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier_from_present_to_clear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier_from_present_to_clear.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier_from_present_to_clear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier_from_present_to_clear.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_clear.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_present_to_clear.image = swapChainInfo.images[swapChainInfo.acquiredImageIndex];
  barrier_from_present_to_clear.subresourceRange = image_subresource_range;

  vkCmdPipelineBarrier( commandBuffer, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_clear );

  VkClearColorValue clear_color = {};
  clear_color.float32[0] = r;
  clear_color.float32[1] = g;
  clear_color.float32[2] = b;
  clear_color.float32[3] = 1.0f;

  vkCmdClearColorImage( commandBuffer,
	                    swapChainInfo.images[swapChainInfo.acquiredImageIndex],
	                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                    &clear_color,
	                    1,
	                    &image_subresource_range );

  VkImageMemoryBarrier barrier_from_clear_to_present = {};
  barrier_from_clear_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_from_clear_to_present.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // VK_ACCESS_MEMORY_READ_BIT fails validation
  barrier_from_clear_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier_from_clear_to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier_from_clear_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier_from_clear_to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_clear_to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier_from_clear_to_present.image = swapChainInfo.images[swapChainInfo.acquiredImageIndex];
  barrier_from_clear_to_present.subresourceRange = image_subresource_range;

  vkCmdPipelineBarrier( commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_clear_to_present );

}

void LsRenderer::DrawLine(float x1, float y1, float x2, float y2) {
  if( !drawingContext.drawing ) {
    BeginDrawing();
  }

  if(drawingContext.pipelineBinding != PipelineBinding::eLine) {
    // Bind line graphics pipeline
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline );
    drawingContext.pipelineBinding = PipelineBinding::eLine; 
  }

  glm::vec3 vulkanPoint1 = windowToVulkanTransformation*glm::vec3(x1, y1, 1.0f);
  glm::vec3 vulkanPoint2 = windowToVulkanTransformation*glm::vec3(x2, y2, 1.0f);

  // Transition image layout from generic read/present
  LinePushConstants pushConstants;
  pushConstants.positions[0] = vulkanPoint1[0];
  pushConstants.positions[1] = vulkanPoint1[1];
  pushConstants.positions[2] = vulkanPoint2[0];
  pushConstants.positions[3] = vulkanPoint2[1];
  std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

  vkCmdPushConstants( commandBuffer, 
	                  linePipelineLayout,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, // offset
                      sizeof(LinePushConstants),
                      &pushConstants );
  vkCmdSetLineWidth( commandBuffer, drawingContext.lineWidth );
  vkCmdDraw( commandBuffer, 2, 1, 0, 0 );
}

void LsRenderer::DrawLine(glm::vec2 p1, glm::vec2 p2)
{
  DrawLine(p1[0], p1[1], p2[0], p2[1]);
}

void LsRenderer::DrawPoint(float x, float y) {
  if( !drawingContext.drawing ) {
    BeginDrawing();
  }

  if(drawingContext.pipelineBinding != PipelineBinding::ePoint) {
    // Bind line graphics pipeline
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline );
    drawingContext.pipelineBinding = PipelineBinding::ePoint;
  }

  glm::vec3 vulkanPoint = windowToVulkanTransformation*glm::vec3(x, y, 1.0f);

  // Transition image layout from generic read/present
  // TODO: Do I need to push it every time?
  PointPushConstants pushConstants;
  pushConstants.positions[0] = vulkanPoint[0];
  pushConstants.positions[1] = vulkanPoint[1];
  pushConstants.size = drawingContext.pointSize;
  std::copy(std::begin(drawingContext.color), std::end(drawingContext.color), std::begin(pushConstants.color));

  vkCmdPushConstants( commandBuffer, 
	                  pointPipelineLayout,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, // offset
                      sizeof(PointPushConstants),
                      &pushConstants );
  vkCmdDraw( commandBuffer, 1, 1, 0, 0 );
}

void LsRenderer::SetColor(float r, float g, float b) {
  drawingContext.color[0] = r;
  drawingContext.color[1] = g;
  drawingContext.color[2] = b;
}

void LsRenderer::SetLineWidth(float width) {
  drawingContext.lineWidth = width;
}

void LsRenderer::SetPointSize(float size) {
  drawingContext.pointSize = size;
}

void LsRenderer::BeginDrawing() {
  assert(!drawingContext.drawing);

  // If queue present and graphics queue families are not the same
  // transfer ownership to graphics queue
  VkImageSubresourceRange image_subresource_range = {
	VK_IMAGE_ASPECT_COLOR_BIT,       // VkImageAspectFlags                     aspectMask
	0,                               // uint32_t                               baseMipLevel
	1,                               // uint32_t                               levelCount
	0,                               // uint32_t                               baseArrayLayer
	1                                // uint32_t                               layerCount
  };

  // wait for writes from clears and draws, block draws
  VkImageMemoryBarrier barrier_from_present_to_draw = {
	VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	nullptr,
	0,                                                      // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                   // VkAccessFlags            dstAccessMask
	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            oldLayout
	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            newLayout
	presentQueue.familyIndex,                               // uint32_t                 srcQueueFamilyIndex
	graphicsQueue.familyIndex,                              // uint32_t                 dstQueueFamilyIndex
	swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
	image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
  };

  vkCmdPipelineBarrier( commandBuffer,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_draw );

  // Begin rendering pass
  VkRenderPassBeginInfo render_pass_begin_info = {
    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	nullptr,
    renderPass,                                     // VkRenderPass                   renderPass
    framebuffers[swapChainInfo.acquiredImageIndex], // VkFramebuffer                  framebuffer
    {                                               // VkRect2D                       renderArea
      {                                             // VkOffset2D                     offset
        0,                                          // int32_t                        x
        0                                           // int32_t                        y
      },
      {                                             // VkExtent2D                     extent
        swapChainInfo.extent.width,                 // int32_t                        width
        swapChainInfo.extent.height,                // int32_t                        height
      }
    },
    0,                                              // uint32_t                       clearValueCount
    nullptr                                         // const VkClearValue            *pClearValues
  };

  vkCmdBeginRenderPass( commandBuffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );

  // Tell drawing functions that command buffer is already prepared for drawing
  drawingContext.drawing = true;

  drawingContext.pipelineBinding = PipelineBinding::eNone;
}

void LsRenderer::EndDrawing() {
  vkCmdEndRenderPass(commandBuffer);
  
  VkImageSubresourceRange image_subresource_range = {
	VK_IMAGE_ASPECT_COLOR_BIT,       // VkImageAspectFlags                     aspectMask
	0,                               // uint32_t                               baseMipLevel
	1,                               // uint32_t                               levelCount
	0,                               // uint32_t                               baseArrayLayer
	1                                // uint32_t                               layerCount
  };

  VkImageMemoryBarrier barrier_from_present_to_draw = {
	VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	nullptr,
	VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                   // VkAccessFlags            srcAccessMask, eMemoryRead fails validation
	VK_ACCESS_MEMORY_READ_BIT,                              // VkAccessFlags            dstAccessMask
	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            oldLayout
	VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                        // VkImageLayout            newLayout
	graphicsQueue.familyIndex,                              // uint32_t                 srcQueueFamilyIndex
	presentQueue.familyIndex,                               // uint32_t                 dstQueueFamilyIndex
	swapChainInfo.images[swapChainInfo.acquiredImageIndex], // VkImage                  image
	image_subresource_range                                 // VkImageSubresourceRange  subresourceRange
  };

  vkCmdPipelineBarrier( commandBuffer, 
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // need to block presentation, there's no actual stage for it,
                        0,                                    // but bottom of pipe is what we need
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &barrier_from_present_to_draw );

  drawingContext.drawing = false;
}

//void LsRenderer::OnWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
//  switch (uMsg) {
//    case WM_SIZE:
//    RefreshSwapChain();
//    windowToVulkanTransformation = WindowToVulkanTransformation(window);
//    break;
//  }
//}//===============================================================================
// @ LsBccLattice.h
// 
// Body Centered Cubic Lattice data structure
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsBCCLatticeTypes.h>
#include <LsBCCLattice.h>
#include <LsMath.h>
#include <assert.h>
#include <cmath>

#include <iostream> // test only

//-------------------------------------------------------------------------------
//-- Typedefs -------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsBCCNodeOffset
//-------------------------------------------------------------------------------
// Specifies displacement between two nodes
//-------------------------------------------------------------------------------
typedef std::tuple<int, int, int> LsBCCNodeOffset;

//-------------------------------------------------------------------------------
//-- Constants -------------------------------------------------------------------
//-------------------------------------------------------------------------------

static const LsBCCNodeOffset nexusOffsets[7] = {
  {1,1,1},
  {1,1,-1},
  {-1,1,-1},
  {-1,1,1},
  {2,0,0},
  {0,2,0},
  {0,0,2}
};

static const LsBCCNodeOffset adjacentOffsets[14] = {
  {1,1,1},
  {1,1,-1},
  {-1,1,-1},
  {-1,1,1},
  {2,0,0},
  {0,2,0},
  {0,0,2},
  {-1,-1,-1},
  {-1,-1,1},
  {1,-1,1},
  {1,-1,-1},
  {-2,0,0},
  {0,-2,0},
  {0,0,-2}
};

// We iterate all tetrahedra following a tiling pattern. The pattern has it's origin at some black node.
// The pattern has three axes along x, y and z with with adjacent black nodes at 2 units offsets.
// Surrounding each of those 3 axes are 4 tetrahedra, making up a pattern of total 12 tetrahedra.
// To iterate over all tetrahedra we iterate over all black vertecies and then over every tetrahedra in
// the pattern.
static const LsBCCNodeOffset tetrahedraOffsets[12][3] = {
  {{2,0,0},{1,1,-1},{1,-1,-1}}, // x-axis
  {{2,0,0},{1,1,1},{1,1,-1}},
  {{2,0,0},{1,-1,1},{1,1,1}},
  {{2,0,0},{1,-1,-1},{1,-1,1}},
  {{0,2,0},{-1,1,-1},{1,1,-1}}, // y-axis
  {{0,2,0},{1,1,1},{1,1,-1}},
  {{0,2,0},{-1,1,1},{1,1,1}},
  {{0,2,0},{-1,1,1},{-1,1,-1}},
  {{0,0,2},{-1,1,1},{1,1,1}},   // z-axis
  {{0,0,2},{1,1,1},{1,-1,1}},
  {{0,0,2},{-1,-1,1},{1,-1,1}},
  {{0,0,2},{-1,1,1},{-1,-1,1}}
};

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

LsBCCNodeOffset SubtractNodes(LsBCCNode const & n1, LsBCCNode const & n2) {
  return LsBCCNodeOffset(std::get<0>(n1) - std::get<0>(n2), std::get<1>(n1) - std::get<1>(n2), std::get<2>(n1) - std::get<2>(n2));
}

LsBCCNode AddNodeOffset(LsBCCNode const & node, LsBCCNodeOffset const & offset) {
  return LsBCCNode(std::get<0>(node) + std::get<0>(offset), std::get<1>(node) + std::get<1>(offset), std::get<2>(node) + std::get<2>(offset));
}

bool NodesEqual(LsBCCNode const & n1, LsBCCNode const & n2) {
  return std::get<0>(n1) == std::get<0>(n2) && std::get<1>(n1) == std::get<1>(n2) && std::get<2>(n1) == std::get<2>(n2);
}

bool NodeOffsetsEqual(LsBCCNodeOffset const & o1, LsBCCNodeOffset const & o2) {
  return std::get<0>(o1) == std::get<0>(o2) && std::get<1>(o1) == std::get<1>(o2) && std::get<2>(o1) == std::get<2>(o2);
}

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

LsBCCLattice::NodeIterator::NodeIterator(LsBCCLattice const& lattice):lattice(&lattice) {
  current = this->lattice->nodeMetaData[currentIndex].coordinates;
};

//LsBCCLattice::NodeIterator const& LsBCCLattice::NodeIterator::operator=(LsBCCLattice::NodeIterator const& other) {
//  this->lattice = other.lattice;
//}

LsBCCLattice::NodeIterator::operator LsBCCNode() const {
  return current;
}

bool LsBCCLattice::NodeIterator::Next() {
  ++currentIndex;
  if ( currentIndex < lattice->nodeMetaData.size() )
  {
    current = lattice->nodeMetaData[currentIndex].coordinates;
    return true; 
  } else {
    return false;
  }
}

LsBCCLattice::TetrahedronIterator::TetrahedronIterator(LsBCCLattice const& lattice):lattice(&lattice) {
  // Iteration pattern goes in steps of two, so to make sure all tetrahedra get the coverage
  // we have to expand lattice bounds into even, if they already aren't.
  minima.x = std::get<0>(this->lattice->minima);
  minima.y = std::get<1>(this->lattice->minima);
  minima.z = std::get<2>(this->lattice->minima);
  if (LsOdd(minima.x)) minima.x = std::get<0>(this->lattice->minima) - 1;
  if (LsOdd(minima.y)) minima.y = std::get<1>(this->lattice->minima) - 1;
  if (LsOdd(minima.z)) minima.z = std::get<2>(this->lattice->minima) - 1;
  maxima.x = std::get<0>(this->lattice->maxima);
  maxima.y = std::get<1>(this->lattice->maxima);
  maxima.z = std::get<2>(this->lattice->maxima);
  if (LsOdd(maxima.x)) maxima.x = std::get<0>(this->lattice->maxima) + 1;
  if (LsOdd(maxima.y)) maxima.y = std::get<1>(this->lattice->maxima) + 1;
  if (LsOdd(maxima.z)) maxima.z = std::get<2>(this->lattice->maxima) + 1;
  currentNode.x = minima.x; // It's normal if the currentNode is not part of the lattice, at the boundaries we have to
  currentNode.y = minima.y; // outside the grid to apply the pattern. The node mast be valid though(all even or all odd coordinates).
  currentNode.z = minima.z;
  assert(this->lattice->Valid(std::make_tuple(currentNode.x, currentNode.y, currentNode.z)));
  currentPatternIndex = -1;
  Next();
};

LsBCCLattice::TetrahedronIterator::operator LsBCCTetrahedron() const {
  return currentTetrahedron;
}

void printNode(LsBCCNode& node) {
  std::cout << "(" << std::get<0>(node) << ", " << std::get<1>(node) << ", " << std::get<2>(node) << ") ";
}

bool LsBCCLattice::TetrahedronIterator::Next() {
  ++currentPatternIndex;
  if ( currentPatternIndex < 12 ) {
    LsBCCNode n1 = { currentNode.x, currentNode.y, currentNode.z };
    LsBCCNode n2 = AddNodeOffset(n1, tetrahedraOffsets[currentPatternIndex][0]);
    LsBCCNode n3 = AddNodeOffset(n1, tetrahedraOffsets[currentPatternIndex][1]);
    LsBCCNode n4 = AddNodeOffset(n1, tetrahedraOffsets[currentPatternIndex][2]);
    currentTetrahedron = { n1, n2, n3, n4 };
    if (lattice->NodeExists(n1) && lattice->NodeExists(n2) && lattice->NodeExists(n3) && lattice->NodeExists(n4)) {
      return true;
    }
    else {
      return Next(); // Try to find another node
    }
  } else {
    currentNode.x += 2;
    if (currentNode.x > maxima.x) {
      currentNode.x = minima.x;
      currentNode.y += 2;
      if (currentNode.y > maxima.y) {
        currentNode.y = minima.y;
        currentNode.z += 2;
        if (currentNode.z > maxima.z) {
          return false; // No more nodes
        }
      }
    }
    currentPatternIndex = -1;
    return Next();
  }
};

LsBCCLattice::NodeEdgeIterator::NodeEdgeIterator(LsBCCLattice const& lattice, LsBCCNode node):lattice(&lattice),n1(node) {
  assert( this->lattice->NodeExists(node) );
  currentAdjacentIndex = -1; // Use Next() method to find first valid edge,
  Next();                    // in case if first edge doesn't exist
};

//-------------------------------------------------------------------------------
// @ LsBCCLattice::NodeEdgeIterator::operator LsBCCEdge() 
//-------------------------------------------------------------------------------
// Get the current edge. The origin node is guaranteed to be the first node of the edge.
//-------------------------------------------------------------------------------
LsBCCLattice::NodeEdgeIterator::operator LsBCCEdge() const {
  return current; //TODO: assert that this is valid
}

bool LsBCCLattice::NodeEdgeIterator::Next() {
  ++currentAdjacentIndex;
  if ( currentAdjacentIndex < 14 )
  {
    LsBCCNode n2 = AddNodeOffset(n1, adjacentOffsets[currentAdjacentIndex]);
    current = LsBCCEdge(n1, n2);
    if (lattice->NodeExists(n2))
    {
      return true;
    }
    return Next(); // Keep looking for next valid edge
  } else {
    return false;
  }
}

LsBCCLattice::EdgeIterator::EdgeIterator(LsBCCLattice const& lattice):lattice(&lattice), nodeIterator(lattice.GetNodeIterator()) {
  currentNexusIndex = -1; 
  Next();
}

LsBCCLattice::EdgeIterator::operator LsBCCEdge() const {
  return current;
}

bool LsBCCLattice::EdgeIterator::Next() {
  ++currentNexusIndex;
  if ( currentNexusIndex < 7 ) {
    LsBCCNode n1 = nodeIterator;
    LsBCCNode n2 = AddNodeOffset(n1, nexusOffsets[currentNexusIndex]);
    current = LsBCCEdge(n1, n2);
    if (lattice->NodeExists(n2))
    {
      return true;
    }
    return Next(); // Keep looking for next valid edge
  }
  else {
    if ( nodeIterator.Next() ) {
      currentNexusIndex = -1; // Recursive call to Next() will make it 0
      return Next();
    }
    else {
      return false; // No more nodes
    }
  }
}

LsBCCLattice::LsBCCLattice(LsDomain domain, float step) {
  // TODO: Implement this in terms of previous constructor code. Domain is a floating 
  // point bounding box. The grid must include every tetrahedron that is touched by it.
  // It can be accomplished simply by rounding up by step size outwards.
  std::tuple<int, int, int> minima = { floor(domain.x1 / step)-1, floor(domain.y1 / step)-1, floor(domain.z1 / step)-1 };
  std::tuple<int, int, int> maxima = { ceil(domain.x2 / step)+1, ceil(domain.y2 / step)+1, ceil(domain.z2 / step)+1 };

  CreateLattice(minima, maxima, step); // TODO: floor, ceiling, round up and down to even
}

//-------------------------------------------------------------------------------
// @ LsBCCLattice::LsBCCLattice()
//-------------------------------------------------------------------------------
// Initialize lattice bounded by inclusive interval
//-------------------------------------------------------------------------------
LsBCCLattice::LsBCCLattice(std::tuple<int, int, int> minima,
                           std::tuple<int, int, int> maxima,
                           float step) {
  CreateLattice(minima, maxima, step);
}

LsBCCLattice::TetrahedronIterator LsBCCLattice::GetTetrahedronIterator() const {
  return TetrahedronIterator(*this);
}

LsBCCLattice::NodeIterator LsBCCLattice::GetNodeIterator() const {
  return NodeIterator(*this);
}

LsBCCLattice::NodeEdgeIterator LsBCCLattice::GetNodeEdgeIterator(LsBCCNode node) const {
  return NodeEdgeIterator(*this, node);
}

LsBCCLattice::EdgeIterator LsBCCLattice::GetEdgeIterator() const {
  return EdgeIterator(*this);
}

glm::vec3 LsBCCLattice::GetNodePosition(LsBCCNode node) const {
  return GetNodeMetaDataConstReference(node).position;
}

LsBCCColor LsBCCLattice::GetNodeColor(LsBCCNode node) const {
  assert(Valid(node));
  if ( LsEven(std::get<0>(node)) ) // Node is black iff all of it's coordinates are even
    return LsBCCColor::eBlack;
  else
    return LsBCCColor::eRed;
}

LsBCCValue LsBCCLattice::GetNodeValue(LsBCCNode node) const {
  assert(Valid(node));
  return GetNodeMetaDataConstReference(node).value;
}

void LsBCCLattice::SetNodeValue(LsBCCNode node, LsBCCValue value) {
  assert(Valid(node));
  GetNodeMetaDataReference(node).value = value;
}

void LsBCCLattice::SetNodePosition(LsBCCNode node, glm::vec3 position) {
  assert(Valid(node));
  GetNodeMetaDataReference(node).position = position;
}

void LsBCCLattice::DeleteNodeCutPoints(LsBCCNode node) {
  assert(Valid(node));
  NodeEdgeIterator iterator = GetNodeEdgeIterator(node);
  do {
    LsBCCEdge edge = iterator;
    GetEdgeMetaDataReference(edge);
    GetEdgeMetaDataReference(edge).cutPoint = LsOptional<glm::vec3>::None();;
  } while ( iterator.Next() );
}
 
LsOptional<glm::vec3> LsBCCLattice::GetEdgeCutPoint(LsBCCEdge edge) const {
  return GetEdgeMetaDataConstReference(edge).cutPoint;
}

void LsBCCLattice::SetEdgeCutPoint(LsBCCEdge edge, glm::vec3 position) {
  LsBCCValue v1 = GetNodeValue(std::get<0>(edge));
  LsBCCValue v2 = GetNodeValue(std::get<1>(edge));
  assert( v1 == LsBCCValue::ePositive && v2 == LsBCCValue::eNegative  ||
          v1 == LsBCCValue::eNegative && v2 == LsBCCValue::ePositive );
  GetEdgeMetaDataReference(edge).cutPoint = position;
}

LsBCCColor LsBCCLattice::GetEdgeColor(LsBCCEdge edge) const {
  // Edge is black if both it's points are red or both it's points are black, otherwise it's red
  LsBCCColor color1 = GetNodeColor(std::get<0>(edge));
  LsBCCColor color2 = GetNodeColor(std::get<1>(edge));
  if ( color1 == LsBCCColor::eRed && color2 == LsBCCColor::eRed )
  {
    return LsBCCColor::eBlack; 
  } else if ( color1 == LsBCCColor::eBlack && color2 == LsBCCColor::eBlack ) {
    return LsBCCColor::eBlack;
  } else {
    return LsBCCColor::eRed;
  }
}

void LsBCCLattice::CreateLattice(std::tuple<int, int, int> minima,
  std::tuple<int, int, int> maxima,
  float step) {
  this->minima = minima;
  this->maxima = maxima;
  for (int z = std::get<2>(minima); z <= std::get<2>(maxima); ++z)
  {
    for (int y = std::get<1>(minima); y <= std::get<1>(maxima); ++y)
    {
      for (int x = std::get<0>(minima); x <= std::get<0>(maxima); ++x)
      {
        LsBCCNode node = { x,y,z };
        if (Valid(node))
        {
          NodeMetaData nodeInfo;
          nodeInfo.coordinates = node;
          nodeInfo.position = step*glm::vec3(x, y, z); // Vertex coordinates correspond to 
          nodeInfo.value = LsBCCValue::eUnassigned;    // BCC grid coordinates scaled by step
          nodeMetaData.push_back(nodeInfo);
        }
      }
    }
  }
}

LsOptional<int> LsBCCLattice::GetEdgeIndexInNexus(LsBCCEdge edge) const {
  // Get vector representing an edge
  LsBCCNodeOffset offset = SubtractNodes(std::get<1>(edge), std::get<0>(edge));

  // Test if edge matches one of nexus pattern edges
  for (int i = 0; i < 7; ++i)
  {
    if( NodeOffsetsEqual(nexusOffsets[i], offset) ) {
      return i;
    }
  }
  return LsOptional<int>::None();
}

//-------------------------------------------------------------------------------
// @ LsBCCLattice::FindEdgeInNexus()
//-------------------------------------------------------------------------------
// Find out nexus node and nexus offset of an edge
//-------------------------------------------------------------------------------
void LsBCCLattice::FindEdgeInNexus(LsBCCEdge edge, LsBCCNode* nexusNode, int* nexusOffset) const {
  // Get vector representing an edge, assume first node is nexus
  LsBCCNodeOffset offset = SubtractNodes(std::get<1>(edge), std::get<0>(edge));

  // Test if edge matches one of nexus pattern edges
  for (int i = 0; i < 7; ++i)
  {
    if( NodeOffsetsEqual(nexusOffsets[i], offset) ) {
      if ( nexusNode ) *nexusNode = std::get<0>(edge);
      if ( nexusOffset ) *nexusOffset = i;
      return;
    }
  }

  // If still not found, assume second node is nexus
  offset = SubtractNodes(std::get<0>(edge), std::get<1>(edge));
  for (int i = 0; i < 7; ++i)
  {
    if( NodeOffsetsEqual(nexusOffsets[i], offset) ) {
      if ( nexusNode ) *nexusNode = std::get<1>(edge);
      if ( nexusOffset ) *nexusOffset = i;
      return;
    }
  }
  assert(false);
}

LsBCCLattice::NodeMetaData& LsBCCLattice::GetNodeMetaDataReference(LsBCCNode node) {
  return nodeMetaData[GetNodeIndex(node)];
}

LsBCCLattice::NodeMetaData const& LsBCCLattice::GetNodeMetaDataConstReference(LsBCCNode node) const {
  return nodeMetaData[GetNodeIndex(node)];
}

LsBCCLattice::EdgeMetaData& LsBCCLattice::GetEdgeMetaDataReference(LsBCCEdge edge) {
  LsBCCNode nexusNode;
  int edgeIndex;
  FindEdgeInNexus(edge, &nexusNode, &edgeIndex);
  return GetNodeMetaDataReference(nexusNode).edgeNexus[edgeIndex];
}

LsBCCLattice::EdgeMetaData const& LsBCCLattice::GetEdgeMetaDataConstReference(LsBCCEdge edge) const {
  LsBCCNode nexusNode;
  int edgeIndex;
  FindEdgeInNexus(edge, &nexusNode, &edgeIndex);
  return GetNodeMetaDataConstReference(nexusNode).edgeNexus[edgeIndex];
}

int LsBCCLattice::GetNodeIndex(LsBCCNode node) const {
  assert(NodeExists(node));
  int evenPlanesCount = LsEvenCount(std::get<2>(minima), std::get<2>(node) - 1);
  int oddPlanesCount = LsOddCount(std::get<2>(minima), std::get<2>(node) - 1);
  int evenPlaneRowSize = LsEvenCount(std::get<0>(minima), std::get<0>(maxima));
  int oddPlaneRowSize = LsOddCount(std::get<0>(minima), std::get<0>(maxima));
  int evenPlaneRowCount = LsEvenCount(std::get<1>(minima), std::get<1>(maxima));
  int oddPlaneRowCount = LsOddCount(std::get<1>(minima), std::get<1>(maxima));
  int evenPlaneSize = evenPlaneRowCount*evenPlaneRowSize;
  int oddPlaneSize = oddPlaneRowCount*oddPlaneRowSize;
  int planeOffset = evenPlaneSize*evenPlanesCount + oddPlaneSize*oddPlanesCount;
  int rowOffset;
  int columnOffset;
  if ( LsEven(std::get<2>(node)) ) {
    rowOffset = LsEvenCount(std::get<1>(minima), std::get<1>(node) - 1)*evenPlaneRowSize;
    columnOffset = LsEvenCount(std::get<0>(minima), std::get<0>(node) - 1);
  } else {
    rowOffset = LsOddCount(std::get<1>(minima), std::get<1>(node) - 1)*oddPlaneRowSize;
    columnOffset = LsOddCount(std::get<0>(minima), std::get<0>(node) - 1);
  }
  return planeOffset + rowOffset + columnOffset;
}

bool LsBCCLattice::WithinBounds(LsBCCNode node) const {
  if (std::get<0>(node) == 1 && std::get<1>(node) == -9 && std::get<2>(node) == -9) {
    std::cout << " ";
  }
  return std::get<0>(node) >= std::get<0>(minima) &&
         std::get<1>(node) >= std::get<1>(minima) &&
         std::get<2>(node) >= std::get<2>(minima) &&
         std::get<0>(node) <= std::get<0>(maxima) &&
         std::get<1>(node) <= std::get<1>(maxima) &&
         std::get<2>(node) <= std::get<2>(maxima);
}

bool LsBCCLattice::Valid(LsBCCNode node) const {
  return LsEven(std::get<0>(node)) && LsEven(std::get<1>(node)) && LsEven(std::get<2>(node)) ||
         LsOdd(std::get<0>(node)) && LsOdd(std::get<1>(node)) && LsOdd(std::get<2>(node));
}

bool LsBCCLattice::NodeExists(LsBCCNode node) const {
  return WithinBounds(node) && Valid(node);
}
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <LsIsosurfaceStuffer.h>
#include <LsBCCLatticeTypes.h>
#include <LsIsosphere.h>
#include <LsMath.h>

LsIsosphere::LsIsosphere() :position({ 0.0f, 0.0f, 0.0f }), radius(0.0f) { };

LsIsosphere::LsIsosphere(glm::vec3 position, float radius):position(position), radius(radius) { };

LsIsosphere::~LsIsosphere() {};

LsDomain LsIsosphere::GetDomain() const {
  LsDomain domain;
  domain.x1 = position.x - radius;
  domain.y1 = position.y - radius;
  domain.z1 = position.z - radius;
  domain.x2 = position.x + radius;
  domain.y2 = position.y + radius;
  domain.z2 = position.z + radius;
  return domain;
}
#include <iostream>
/*
  Evaluate inequality:
  (x-x0)^2 + (y-y0)^2 + (z-z0)^2 < r^2
*/
LsBCCValue LsIsosphere::EvaluateAt(glm::vec3 vertex) const {
  float cutFunction = pow(radius, 2) - (pow(vertex.x - position.x, 2) + pow(vertex.y - position.y, 2) + pow(vertex.z - position.z, 2));
  if ( IsZero(cutFunction) )
  {
    return LsBCCValue::eZero;
  } else if ( cutFunction < 0.0f ) {
    return LsBCCValue::eNegative;
  } else {
    return LsBCCValue::ePositive;
  }
}

/*
  Solve this for t:
  x1 + t*(x2 - x1) = x
  y1 + t*(y2 - y1) = y
  z1 + t*(z2 - z1) = z
  x^2 + y^2 + z^2 = r^2 OR
  (x-x0)^2 + (y-y0)^2 + (z-z0)^2 = r^2
*/
glm::vec3 LsIsosphere::IntersectByEdge(glm::vec3 p1, glm::vec3 p2) const {
  float a = pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2) + pow(p2.z - p1.z, 2);
  float b = 2*((p1.x - position.x)*(p2.x - p1.x) + (p1.y - position.y)*(p2.y - p1.y) + (p1.z - position.z)*(p2.z - p1.z));
  float c = pow(p1.x - position.x, 2) + pow(p1.y - position.y, 2) + pow(p1.z - position.z, 2) - pow(radius, 2);
  float t1 = (-b + sqrt(pow(b,2) - 4*a*c))/(2*a);
  float t2 = (-b - sqrt(pow(b,2) - 4*a*c))/(2*a);
  //std::cout << "Params:" << std::endl;
  //std::cout << t1 << std::endl;
  //std::cout << t2 << std::endl;
  //assert((t1 > 0.0f) || (t2 > 0.0f));
  float t;
  if ( t1 > 0.0f && t1 < 1.0f ) { // Choose positive root that is in 0 to 1 range
    t = t1;
  } else {
    t = t2;
  }
  return p1 + t*(p2 - p1);
}
#include "LsBCCLattice.h"
#include <LsIsosurfaceStuffer.h>
#include <iostream>
#include <unordered_map>
#include <string>

class ILsTetrahedronProcessor {
  
};


template <typename E>
constexpr auto to_underlying(E e) noexcept
{
  return static_cast<std::underlying_type_t<E>>(e);
}

bool operator<(LsBCCValue const& v1, LsBCCValue const& v2) {
  return to_underlying(v1) < to_underlying(v2);
}


// bool operator>(LsBCCValue const& v1, LsBCCValue const& v2) {
//   return to_underlying(v1) > to_underlying(v2);
// }

// bool operator>=(LsBCCValue const& v1, LsBCCValue const& v2) {
//   return to_underlying(v1) >= to_underlying(v2);
// }

// bool operator<=(LsBCCValue const& v1, LsBCCValue const& v2) {
//   return to_underlying(v1) <= to_underlying(v2);
// }

LsIsosurfaceStuffer::LsIsosurfaceStuffer()
{
}

LsIsosurfaceStuffer::~LsIsosurfaceStuffer()
{
}

void LsIsosurfaceStuffer::Stuff(LsTetrahedronMesh& mesh, LsIsosurface const& stuffable) {
  LsBCCLattice bccLattice(stuffable.GetDomain(), step);
  UpdateValues(bccLattice, stuffable);
  UpdateCutPoints(bccLattice, stuffable);
  Warp(bccLattice);
  Fill(bccLattice, mesh);
}

void LsIsosurfaceStuffer::SetAlphaLong(float a)
{
  alphaLong = a;
}

void LsIsosurfaceStuffer::SetAlphaShort(float a)
{
  alphaShort = a;
}

void LsIsosurfaceStuffer::SetStep(float step)
{
  this->step = step;
}

void LsIsosurfaceStuffer::UpdateValues(LsBCCLattice& lattice, LsIsosurface const& stuffable) {
  LsBCCLattice::NodeIterator iterator = lattice.GetNodeIterator();
  do {
    LsBCCNode node = iterator;
    lattice.SetNodeValue(node, stuffable.EvaluateAt(lattice.GetNodePosition(node)));
  } while ( iterator.Next() );
}

void LsIsosurfaceStuffer::UpdateCutPoints(LsBCCLattice& lattice, LsIsosurface const& stuffable) {
  LsBCCLattice::EdgeIterator iterator = lattice.GetEdgeIterator();
  do {
    LsBCCEdge edge = iterator;
    LsBCCNode n1 = std::get<0>(edge);
    LsBCCNode n2 = std::get<1>(edge);
    LsBCCValue v1 = lattice.GetNodeValue(n1);
    LsBCCValue v2 = lattice.GetNodeValue(n2);
    if ( (v1 == LsBCCValue::eNegative && v2 == LsBCCValue::ePositive) ||
         (v1 == LsBCCValue::ePositive && v2 == LsBCCValue::eNegative) ) {
      glm::vec3 p1 = lattice.GetNodePosition(n1);
      glm::vec3 p2 = lattice.GetNodePosition(n2);
      glm::vec3 cutPoint = stuffable.IntersectByEdge(p1, p2); // This edge is guaranteed to have a cut point
      lattice.SetEdgeCutPoint(edge, cutPoint);
    }
  } while ( iterator.Next() );
}

void LsIsosurfaceStuffer::Warp(LsBCCLattice& lattice) {
  LsBCCLattice::NodeIterator nodeIterator = lattice.GetNodeIterator();
  do {
    LsBCCNode n1 = nodeIterator;
    LsBCCLattice::NodeEdgeIterator edgeIterator = lattice.GetNodeEdgeIterator(n1);
    do {
      LsBCCEdge edge = edgeIterator;
      if (!lattice.GetEdgeCutPoint(edge)) {
        continue;
      }
      LsBCCNode n2 = std::get<1>(edge);
      glm::vec3 cutPoint = lattice.GetEdgeCutPoint(edge);
      glm::vec3 p1 = lattice.GetNodePosition(n1);
      glm::vec3 p2 = lattice.GetNodePosition(n2);

      float alpha = alphaLong;
      if ( lattice.GetEdgeColor(edge) == LsBCCColor::eRed )
      {
        alpha = alphaShort;
      }

      if ( glm::abs(glm::length((cutPoint - p1)/(p2 - p1))) < alpha ) // cutPoint violates p1
      {
        lattice.SetNodePosition(n1, cutPoint);       // snap violated node to cut point
        lattice.SetNodeValue(n1, LsBCCValue::eZero); // update value, it lays on surface
        lattice.DeleteNodeCutPoints(n1);             // since it lays on surface, no cut points can be possible
      }
    } while ( edgeIterator.Next() );
  } while ( nodeIterator.Next() );
}

// void LsIsosurfaceStuffer::Fill(LsBCCLattice const& lattice, LsITetrahedronProcessor& processor) {
//   //std::unordered_map<LsBCCNode,int> ids;
//   // hash[v1].terahedra_volume_summ[v2]
//   //         .

//   // hash[v1].stiffness

//   //processor
// }

void LsIsosurfaceStuffer::Fill(LsBCCLattice const& lattice, LsTetrahedronMesh& mesh) {
  std::unordered_map<std::string,int> ids;
  
  LsBCCLattice::TetrahedronIterator iterator = lattice.GetTetrahedronIterator();
  do {
    LsBCCTetrahedron tetrahedron = iterator;   
    if ( pppp.Match(lattice, tetrahedron) ) { // Group 1

      mesh.AddTetrahedron( pppp.GetNodePosition(1),
                           pppp.GetNodePosition(2),
                           pppp.GetNodePosition(3),
                           pppp.GetNodePosition(4) );
    } else if ( zppp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( zppp.GetNodePosition(1),
                           zppp.GetNodePosition(2),
                           zppp.GetNodePosition(3),
                           zppp.GetNodePosition(4) );
    } else if ( zzpp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( zzpp.GetNodePosition(1),
                           zzpp.GetNodePosition(2),
                           zzpp.GetNodePosition(3),
                           zzpp.GetNodePosition(4) );
    } else if ( zzzp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( zzzp.GetNodePosition(1),
                           zzzp.GetNodePosition(2),
                           zzzp.GetNodePosition(3),
                           zzzp.GetNodePosition(4) );
    } else if ( nzzp.Match(lattice, tetrahedron) ) {  // Group 2
      mesh.AddTetrahedron( nzzp.GetEdgeCutPoint(4, 1), // LsCutPoint(GetNodeById(4),GetNodeById(1))
                           nzzp.GetNodePosition(2),
                           nzzp.GetNodePosition(3),
                           nzzp.GetNodePosition(4) );
    } else if ( nnzp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( nnzp.GetEdgeCutPoint(4, 1),
                           nnzp.GetEdgeCutPoint(4, 2),
                           nnzp.GetNodePosition(3),
                           nnzp.GetNodePosition(4) );
    } else if ( nnnp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( nnnp.GetEdgeCutPoint(4, 1),
                           nnnp.GetEdgeCutPoint(4, 2),
                           nnnp.GetEdgeCutPoint(4, 3),
                           nnnp.GetNodePosition(4) );
    } else if ( nnnp.Match(lattice, tetrahedron) ) {
      mesh.AddTetrahedron( nnnp.GetEdgeCutPoint(4, 1),
                           nnnp.GetEdgeCutPoint(4, 2),
                           nnnp.GetEdgeCutPoint(4, 3),
                           nnnp.GetNodePosition(4) );
    } else if ( nzpp.Match(lattice, tetrahedron) ) { // Group 3
      mesh.AddTetrahedron( nzpp.GetNodePosition(2),
                           nzpp.GetNodePosition(3),
                           nzpp.GetNodePosition(4),
                           nzpp.GetEdgeCutPoint(1, 2) );
      mesh.AddTetrahedron( nzpp.GetEdgeCutPoint(1, 3),
                           nzpp.GetNodePosition(3),
                           nzpp.GetNodePosition(4),
                           nzpp.GetEdgeCutPoint(1, 2));
    } else if (nnpp.Match(lattice, tetrahedron)) {
      mesh.AddTetrahedron( nnpp.GetNodePosition(1),
                           nnpp.GetEdgeCutPoint(1, 2),
                           nnpp.GetEdgeCutPoint(3, 4),
                           nnpp.GetEdgeCutPoint(1, 4) );
      mesh.AddTetrahedron( nnpp.GetNodePosition(1),
                           nnpp.GetEdgeCutPoint(1, 2),
                           nnpp.GetNodePosition(3),
                           nnpp.GetEdgeCutPoint(3, 4) );
      mesh.AddTetrahedron( nnpp.GetEdgeCutPoint(2, 3),
                           nnpp.GetEdgeCutPoint(1, 2),
                           nnpp.GetNodePosition(3),
                           nnpp.GetEdgeCutPoint(3, 4) );
    } else if (nnpp_parity.Match(lattice, tetrahedron)) {  // Group 4
      if ( nnpp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nnpp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        std::cout << "g4 nnpp" << std::endl;
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetEdgeCutPoint(2, 3),
                             nnpp_parity.GetEdgeCutPoint(1, 3),
                             nnpp_parity.GetEdgeCutPoint(1, 4) );
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetEdgeCutPoint(1, 4),
                             nnpp_parity.GetEdgeCutPoint(2, 3) );
        mesh.AddTetrahedron( nnpp_parity.GetEdgeCutPoint(2, 3),
                             nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetEdgeCutPoint(1, 4),
                             nnpp_parity.GetEdgeCutPoint(2, 4) );
      } else {
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetEdgeCutPoint(4, 2),
                             nnpp_parity.GetEdgeCutPoint(4, 1),
                             nnpp_parity.GetEdgeCutPoint(1, 3) );
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetEdgeCutPoint(1, 3),
                             nnpp_parity.GetEdgeCutPoint(2, 3),
                             nnpp_parity.GetEdgeCutPoint(4, 2) );
        mesh.AddTetrahedron( nnpp_parity.GetNodePosition(4),
                             nnpp_parity.GetNodePosition(3),
                             nnpp_parity.GetEdgeCutPoint(1, 3),
                             nnpp_parity.GetEdgeCutPoint(4, 2) );
      }
    } else if (nppp_parity.Match(lattice, tetrahedron)) {
      if ( nppp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nppp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        std::cout << "g4 nppp" << std::endl;
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetEdgeCutPoint(1, 4),
                             nppp_parity.GetEdgeCutPoint(1, 2),
                             nppp_parity.GetEdgeCutPoint(1, 3) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetNodePosition(2),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetEdgeCutPoint(1, 4),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
      } else {
        std::cout << "g4 nppp flip" << std::endl;
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(4),
                             nppp_parity.GetEdgeCutPoint(1, 4),
                             nppp_parity.GetEdgeCutPoint(1, 2),
                             nppp_parity.GetEdgeCutPoint(1, 3) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetNodePosition(2),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
        mesh.AddTetrahedron( nppp_parity.GetNodePosition(3),
                             nppp_parity.GetNodePosition(4),
                             nppp_parity.GetEdgeCutPoint(1, 3),
                             nppp_parity.GetEdgeCutPoint(1, 2) );
      }
    } else if (nzpp_parity.Match(lattice, tetrahedron)) {
      if ( nzpp_parity.GetNodeColor(3) == LsBCCColor::eBlack &&
           nzpp_parity.GetNodeColor(4) == LsBCCColor::eBlack ) {
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 4),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(3) );
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 4),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(3) );
      }
      else {
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 4),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(4) );
        mesh.AddTetrahedron( nzpp_parity.GetNodePosition(3),
                             nzpp_parity.GetNodePosition(2),
                             nzpp_parity.GetEdgeCutPoint(1, 3),
                             nzpp_parity.GetNodePosition(4) );
      }
    }
  } while ( iterator.Next() );
}

/* Graph matching algorithm

1 is +
2 is +
3 is +
3 is +
any edge 1 2
any edge 2 3
any edge 3 1
any edge 4 1
any edge 4 2
any edge 4 3

1 is -
2 is +
black edge 1 2 cut
3 is +
red edge 1 3 cut
red edge 3 2
4 is 0
black edge 4 3
red edge 4 2
red edge 4 1


Implicitly assign cut points an id based on nodes of the edge
GetPosition(1);
GetPosition(1,2);
GetNode(1);
GetNode(1,2); // ERROR: no such function

auto it = lattice.GetNodeIterator();
lattice.GetNodeValue(it) == LsBCCValue::eNegative; // 1
matches[1] = it;

// find all positive
lattice.GetNodeValue(it) == LsBCCValue::ePositive; // 2
// recur for each

- 0 + +
0 1 2 3

1 n0
2 n2, n3
3 n2, n3
4 n1

1 is +
2 is -
black edge 1 2 cut
3 is +
red edge 2 3 cut
red edge 1 3
4 is -
red edge 4 1 cut
black edge 4 3 cut
red edge 4 2

- - + +
0 1 2 3

1 n2, n3
2 n2, n4
3 n2, n3
4 n2, n4

n2, n3 are good candidates, because they are +'ses
look for all edges that have the 1 in them:

black edge 1 2 cut
red edge 1 3
red edge 4 1 cut

we cannot make a decision right now, because we have not selected 2, 3, 4
but if did select 2, 3, 4

lattice.GetEdgeColor(<edge between 1 and 2>) == black;
lattice.GetEdgeCutPoint(<edge between 1 and 2>) == <not Nothing>;
lattice.GetEdgeColor(<edge between 1 and 3>) == red;
lattice.GetEdgeCutPoint(<edge between 1 and 3>) == <Nothing>;
lattice.GetEdgeColor(<edge between 1 and 4>) == red;
lattice.GetEdgeCutPoint(<edge between 1 and 4>) == <not Nothing>;

// select nodes that are required to check the edges
// if it cannot do that, fail
// if you can 

// make a choice
// recur with the choice
// until no more choices need to be made

// check each possible choice of node 4 choices, each can be 4 different things
// choose 1 as n0, n1, n2, n3
//   choose 2 as n1, n2, n3
//     choose 3 as n2, n3
//       choose 4 as n3
//         check edges

// start with n0, remainding n1,n2,n3
//

// class LsStencil
// {
// public:
//   LsStencil();
//   ~LsStencil();
//   Get
// };

*/

#include <LsOptional.h>

using NodeSelections = std::vector<LsBCCNode>;

bool match(LsBCCLattice const& lattice, 
                                 LsBCCTetrahedron const& tetrahedron,
                                 std::vector<LsBCCNode> selected, 
                                 std::vector<LsBCCNode> remaining,
                                 int index) {
  // n cases
  // n = 3 - index; worsk size of NEXT recursion
  // 4 - index
  // 4 0
  // 3 1
  // 2 2
  // 1 3
  // 0 4
  return false;
}

// template<class T>
// void find_permutation_if(std::vector<T> elements, bool (*predicate)(std::vector<T>const& permutation)) {

// }

#include <FactoradicPermutation.hh>
#include <string>
#include <iostream>

void test() {
  std::string x = "abcd";
  NthPermutation(x.begin(), x.end(), 2);
  std::cout << x << std::endl;
}

class LsPattern
{
public:
  LsPattern();
  ~LsPattern();
  void AddNode(int id);
  void AddEdge(int id);

  
};

class PatternDSL;

struct _id_data_ {
  int id;
  LsBCCValue value;
};

class _is_
{
  PatternDSL& mPattern;
public:
  _is_(PatternDSL& pattern):mPattern(pattern) {};
  ~_is_() {};
  PatternDSL& plus();
};

class _id_
{
  PatternDSL& mPattern;
  int mId;
public:
  _id_(PatternDSL& pattern, int id):mPattern(pattern),mId(id){};
  ~_id_();
  _is_ is();
};

class IPatternDSLInterpreter
{
public:
  IPatternDSLInterpreter();
  ~IPatternDSLInterpreter();
  virtual void OnId(int id, LsBCCValue value) = 0;
  virtual void OnEdge(int id1, int id2, LsBCCColor color, bool isCut) = 0;
};

class PatternDSL
{
friend ::_id_;
friend ::_is_;
  IPatternDSLInterpreter& mInterpreter;
  IPatternDSLInterpreter& GetInterpreter();
public:
  PatternDSL(IPatternDSLInterpreter& interpreter):mInterpreter(interpreter){};
  ~PatternDSL();
  _id_ id(int _id);
};

_is_ _id_::is() {
    return ::_is_(mPattern);
};

PatternDSL& _is_::plus() {
  mPattern.GetInterpreter().OnId(0, LsBCCValue::ePositive);
  return mPattern;
}

IPatternDSLInterpreter& PatternDSL::GetInterpreter() {
  return mInterpreter;
}

_id_ PatternDSL::id(int id) {
  return ::_id_(*this, id);
}

void aaa() {
  // PatternDSL pattern;
  // pattern
  // .id(1).is().plus()
  // .id(2).is().plus()
  // .red().edge(1,2).cut()
}

using LsBCCMaybeColor = LsOptional<LsBCCColor>;

class PatternMatcher // NOTE: we don't need to mark cut, + - edges are always cut, everything else is not
{
private:
  LsBCCNode matchedNodes[4];
  std::tuple<int, LsBCCValue> patternNodes[4];
  int currentNodeIndex = 0;
  std::tuple<int, int, LsBCCMaybeColor> patternEdges[6];
  int currentEdgeIndex = 0;
  LsBCCLattice const* pLattice;
protected:
  void AddNode(int id, LsBCCValue value);
  void AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor);
public:
  PatternMatcher() {};
  ~PatternMatcher() {};
  bool Match(LsBCCLattice const& lattice, LsBCCTetrahedron tetrahedron);
  LsBCCNode GetNodeById(int id);
  glm::vec3 GetNodePosition(int id);             // Convenience functions
  glm::vec3 GetEdgeCutPoint(int id1, int id2);
  LsBCCColor GetNodeColor(int id);
  LsBCCColor GetEdgeColor(int id1, int id2);
  LsBCCValue GetNodeValue(int id);
};

void PatternMatcher::AddNode(int id, LsBCCValue value) {
  assert(currentNodeIndex < 4);
  patternNodes[currentNodeIndex++] = {id, value};
}

void PatternMatcher::AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor) {
  assert(currentEdgeIndex < 6);
  patternEdges[currentEdgeIndex++] = {id1, id2, maybeColor};
}

LsBCCNode PatternMatcher::GetNodeById(int id) {
  for (int i = 0; i < 4; ++i)
  {
    if (std::get<0>(patternNodes[i]) == id) {
      return matchedNodes[i];
    }
  }
}

LsBCCColor PatternMatcher::GetEdgeColor(int id1, int id2) {
  return pLattice->GetEdgeColor(LsBCCEdge(GetNodeById(id1), GetNodeById(id2)));
}

bool PatternMatcher::Match(LsBCCLattice const& lattice, LsBCCTetrahedron tetrahedron) {
  pLattice = &lattice;
  
  // Fill permutation array with initial values
  matchedNodes[0] = std::get<0>(tetrahedron); // Node stored at index 0 in matchedNodes
  matchedNodes[1] = std::get<1>(tetrahedron); // corresponds to id stored at patternNodes
  matchedNodes[2] = std::get<2>(tetrahedron); // under same index.
  matchedNodes[3] = std::get<3>(tetrahedron);

  for (int i = 0; i < 24; ++i) // This loop will try to permute matchedNodes until the pattern fits it.
  {
    NthPermutation(&matchedNodes[0], &matchedNodes[4], i); // Integer tuples implement lexicographical comparison.

    bool failed = false;
    
    // Test that nodes have correct values
    for (int j = 0; j < 4; ++j)
    {
      if (lattice.GetNodeValue(matchedNodes[j]) != std::get<1>(patternNodes[j])) {
        failed = true;
        break;
      }
    }

    // Try next permutation
    if (failed) continue;

    // Test edge colors
    for (int j = 0; j < 6; ++j)
    {
      LsBCCMaybeColor patternEdgeColor = std::get<2>(patternEdges[j]);

      if (!patternEdgeColor) continue; // Pattern accepts any color for this edge, no test.

      int id1 = std::get<0>(patternEdges[j]);
      int id2 = std::get<1>(patternEdges[j]);
      if ( GetEdgeColor(id1, id2) != patternEdgeColor ) {
        failed = true;
        break;
      }
    }

    if(!failed) return true; // Found.
  }
  return false; // Not found.
}

class PPPPMatcher: public PatternMatcher {
public:
  PPPPMatcher();
  ~PPPPMatcher() {};
};

PPPPMatcher::PPPPMatcher() {
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

/*
  Node('+', 1) 
  Edge(eRed, 1, 2).Cut(1).
  Edge(eBlack, 1, 2).Cut(1).

Id(1).Is().Plus().
Id(2).Is().Minus().
Black().Edge(1,2).Cut().
Red().Edge(1,2).Cut()

1 is +
2 is -
black edge 1 2 cut
3 is +
red edge 2 3 cut
red edge 1 3
4 is -
red edge 4 1 cut
black edge 4 3 cut
red edge 4 2

matcher01.AddNode(1, ePositive);
matcher01.AddEdge(1, 2, eRed, true);
.id(1).is().plus();


*/
//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <LsOptional.h>
#include <FactoradicPermutation.hh>
#include <LsPatternMatchers.h>

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsPatternMatcher::AddNode(int id, LsBCCValue value) {
  assert(currentNodeIndex < 4);
  patternNodes[currentNodeIndex++] = {id, value};
}

void LsPatternMatcher::AddEdge(int id1, int id2, LsBCCMaybeColor maybeColor) {
  assert(currentEdgeIndex < 6);
  patternEdges[currentEdgeIndex++] = {id1, id2, maybeColor};
}

LsBCCNode LsPatternMatcher::GetNodeById(int id) {
  for (int i = 0; i < 4; ++i)
  {
    if (std::get<0>(patternNodes[i]) == id) {
      return matchedNodes[i];
    }
  }
}

glm::vec3 LsPatternMatcher::GetNodePosition(int id)
{
  return pLattice->GetNodePosition(GetNodeById(id));
}

LsOptional<glm::vec3> LsPatternMatcher::GetEdgeCutPoint(int id1, int id2) {
  return pLattice->GetEdgeCutPoint(LsBCCEdge(GetNodeById(id1), GetNodeById(id2)));
}

LsBCCColor LsPatternMatcher::GetNodeColor(int id)
{
  return pLattice->GetNodeColor(GetNodeById(id));
}

LsBCCColor LsPatternMatcher::GetEdgeColor(int id1, int id2) {
  return pLattice->GetEdgeColor(LsBCCEdge(GetNodeById(id1), GetNodeById(id2)));
}

bool LsPatternMatcher::Match(LsBCCLattice const& lattice, LsBCCTetrahedron tetrahedron) {
  pLattice = &lattice;
  
  // Fill permutation array with initial values
  matchedNodes[0] = std::get<0>(tetrahedron);
  matchedNodes[1] = std::get<1>(tetrahedron);
  matchedNodes[2] = std::get<2>(tetrahedron);
  matchedNodes[3] = std::get<3>(tetrahedron);

  for (int i = 0; i < 24; ++i) // This loop will try to permute matchedNodes until the pattern fits it.
  {
    NthPermutation(&matchedNodes[0], &matchedNodes[4], i); // Integer tuples implement lexicographical comparison.

    bool failed = false;
    
    // Test that nodes have correct values
    for (int j = 0; j < 4; ++j)
    {
      if (lattice.GetNodeValue(matchedNodes[j]) != std::get<1>(patternNodes[j])) {
        failed = true;
        break;
      }
    }

    // Try next permutation
    if (failed) continue;

    // Test edge colors
    for (int j = 0; j < 6; ++j)
    {
      LsBCCMaybeColor patternEdgeColor = std::get<2>(patternEdges[j]);

      if (!patternEdgeColor) continue; // Pattern accepts any color for this edge, no test.

      int id1 = std::get<0>(patternEdges[j]);
      int id2 = std::get<1>(patternEdges[j]);
      if ( GetEdgeColor(id1, id2) != patternEdgeColor ) {
        failed = true;
        break;
      }
    }

    if(!failed) return true; // Found.
  }
  return false; // Not found.
}

LsPPPPMatcher::LsPPPPMatcher() {
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsZPPPMatcher::LsZPPPMatcher() {
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::eZero);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None()); 
}

LsZZPPMatcher::LsZZPPMatcher() {
  AddNode(1, LsBCCValue::eZero);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsZZZPMatcher::LsZZZPMatcher() {
  AddNode(1, LsBCCValue::eZero);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::eZero);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNZZPMatcher::LsNZZPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::eZero);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNNZPMatcher::LsNNZPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::eZero);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNNNPMatcher::LsNNNPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::eNegative);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCMaybeColor::None());
  AddEdge(2, 3, LsBCCMaybeColor::None());
  AddEdge(3, 1, LsBCCMaybeColor::None());
  AddEdge(4, 1, LsBCCMaybeColor::None());
  AddEdge(4, 2, LsBCCMaybeColor::None());
  AddEdge(4, 3, LsBCCMaybeColor::None());
}

LsNZPPMatcher::LsNZPPMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::eZero);
  AddEdge(1, 2, LsBCCColor::eBlack); // Cut red
  AddEdge(3, 4, LsBCCColor::eBlack); 
  AddEdge(3, 2, LsBCCColor::eRed);
  AddEdge(3, 1, LsBCCColor::eRed); // Cut black
  AddEdge(4, 1, LsBCCColor::eRed);
  AddEdge(4, 2, LsBCCColor::eRed);
}

LsNNPPMatcher::LsNNPPMatcher()
{
  AddNode(1, LsBCCValue::ePositive);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::eNegative);
  AddEdge(1, 2, LsBCCColor::eBlack);
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(1, 4, LsBCCColor::eRed);
  AddEdge(1, 3, LsBCCColor::eRed);
  AddEdge(2, 4, LsBCCColor::eRed);
  AddEdge(2, 3, LsBCCColor::eRed);
}

LsNZPPParityMatcher::LsNZPPParityMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eZero);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCColor::eBlack); // Note: This looks like previous NZPP pattern, but nodes have different signs!
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(3, 2, LsBCCColor::eRed);
  AddEdge(3, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 2, LsBCCColor::eRed);
}

LsNPPPParityMatcher::LsNPPPParityMatcher() {
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::ePositive);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCColor::eBlack); // Cut black
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(3, 2, LsBCCColor::eRed);
  AddEdge(3, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 1, LsBCCColor::eRed); // Cut red
  AddEdge(4, 2, LsBCCColor::eRed);
}

LsNNPPParityMatcher::LsNNPPParityMatcher()
{
  AddNode(1, LsBCCValue::eNegative);
  AddNode(2, LsBCCValue::eNegative);
  AddNode(3, LsBCCValue::ePositive);
  AddNode(4, LsBCCValue::ePositive);
  AddEdge(1, 2, LsBCCColor::eBlack);
  AddEdge(3, 4, LsBCCColor::eBlack);
  AddEdge(1, 4, LsBCCColor::eRed); // Cut red
  AddEdge(1, 3, LsBCCColor::eRed); // Cut red
  AddEdge(2, 4, LsBCCColor::eRed); // Cut red
  AddEdge(2, 3, LsBCCColor::eRed); // Cut red
}

//===============================================================================
// @ LsTetrahedronMesh.cpp
// 
// A general class for constructing tetrahedron mesh
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <glm/glm.hpp>
#include <LsMath.h>
#include <LsTetrahedronMesh.h>
#include <LsStdAlgorithms.h>

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

bool doesReferenceNode(LsTetrahedronMesh::LsTetrahedron tetrahedron, int node) {
  return tetrahedron[0] == node ||
         tetrahedron[1] == node ||
         tetrahedron[2] == node ||
         tetrahedron[3] == node;
}

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

LsTetrahedronMesh::LsTetrahedronMesh() {

}

LsTetrahedronMesh::~LsTetrahedronMesh() { }

void LsTetrahedronMesh::UseAbsoluteEq(bool value) {
  useAbsoluteEq = value;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::AddTetrahedron()
//-------------------------------------------------------------------------------
// Add tetrahedron by specifying indecies of it's nodes, duplicate tetrahedra are allowed.
//-------------------------------------------------------------------------------
int LsTetrahedronMesh::AddTetrahedron(int node1, int node2, int node3, int node4) {
  indexBuffer.push_back(LsTetrahedron(node1, node2, node3, node4));
  return indexBuffer.size() - 1;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::AddTetrahedron()
//-------------------------------------------------------------------------------
// Add tetrahedron by specifying coordinates of it's vertecies, duplicate tetrahedra are allowed.
//-------------------------------------------------------------------------------
int LsTetrahedronMesh::AddTetrahedron(const glm::vec3& node1, 
                                      const glm::vec3& node2, 
                                      const glm::vec3& node3, 
                                      const glm::vec3& node4) {
  indexBuffer.push_back(LsTetrahedron(AddNode(node1), AddNode(node2), AddNode(node3), AddNode(node4)));
  return indexBuffer.size() - 1;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::RemoveTetrahedron()
//-------------------------------------------------------------------------------
// Remove tetrahedron by index, may leave some vertecies unused and require a
// call to LsTetrahedronMesh::Optimize() to prune them.
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::RemoveTetrahedron(int tetrahedronIndex) {
  // This might leave some nodes unused, but it's legal to have unused vertecies
  indexBuffer.erase(indexBuffer.begin() + tetrahedronIndex);
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::FindNodeIndex(const glm::vec3&)
//-------------------------------------------------------------------------------
// Try to get index of a node approximately equal to given coordinates
//-------------------------------------------------------------------------------
LsOptional<int> LsTetrahedronMesh::FindNodeIndex(const glm::vec3& node) {
  return find_index_if( vertexBuffer.begin(), vertexBuffer.end(), [&](const glm::vec3& other) {
    if (useAbsoluteEq) {
      return node[0] == other[0] &&
             node[1] == other[1] &&
             node[2] == other[2];
    } else {
      return IvAreEqual(node[0], other[0]) && 
             IvAreEqual(node[1], other[1]) &&
             IvAreEqual(node[2], other[2]);
    }
  });
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::AddNode(const glm::vec3&)
//-------------------------------------------------------------------------------
// Add a new vertex to vertex buffer and return it's index, duplicates are not added.
//-------------------------------------------------------------------------------
int LsTetrahedronMesh::AddNode(const glm::vec3& node) {
  LsOptional<int> index = FindNodeIndex(node);
  if (!index)
  {
    vertexBuffer.push_back(node);
    index = vertexBuffer.size() - 1;
  }
  return index;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::RemoveNode(int)
//-------------------------------------------------------------------------------
// Remove node and possibly any tetrahedra referencing it
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::RemoveNode(int node) {
  // Find all tetrahedra referencing the node and remove them (erase-remove idiom)
  indexBuffer.erase(std::remove_if(indexBuffer.begin(),
                                   indexBuffer.end(),
                                   [&](LsTetrahedron tetrahedron) { return doesReferenceNode(tetrahedron, node); }),
                    indexBuffer.end());
  RemoveUnusedNode(node);
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::GetNodePosition(int)
//-------------------------------------------------------------------------------
// Look-up node position
//-------------------------------------------------------------------------------
glm::vec3 LsTetrahedronMesh::GetNodePosition(int node) {
  return vertexBuffer[node];
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::RemoveUnusedNode(int)
//-------------------------------------------------------------------------------
// Remove vertex from vertex buffer and update index buffer. 
// Requirement: node must not be referenced by any tetrahedra in the index buffer.
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::RemoveUnusedNode(int node) {
  // Remove node(shifts all nodes higher than the removed node to the left)
  vertexBuffer.erase(vertexBuffer.begin() + node);
  // If referenced node is higher than the node that was removed subtract 1 from it
  for(LsTetrahedron& tetrahedron:indexBuffer) {
    if (tetrahedron[0] > node) tetrahedron[0]--;
    if (tetrahedron[1] > node) tetrahedron[1]--;
    if (tetrahedron[2] > node) tetrahedron[2]--;
    if (tetrahedron[3] > node) tetrahedron[3]--;
  }
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::Optimize()
//-------------------------------------------------------------------------------
// Prune vertex buffer from unused vertecies
//-------------------------------------------------------------------------------
void LsTetrahedronMesh::Optimize() {
  // Count how many times each node is referenced
  std::vector<int> nodeUseCount(vertexBuffer.size(), 0);
  for(LsTetrahedron& tetrahedron:indexBuffer) {
    nodeUseCount[tetrahedron[0]]++;
    nodeUseCount[tetrahedron[1]]++;
    nodeUseCount[tetrahedron[2]]++;
    nodeUseCount[tetrahedron[3]]++;
  }
  // Remove all unused nodes
  int i = 0;
  while(i < vertexBuffer.size()) {
    if (!nodeUseCount[i])
    {
      RemoveUnusedNode(i);
      nodeUseCount.erase(nodeUseCount.begin() + i);
      // Do not increment i!
    } else {
      i++;
    }
  }
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::GetVertecies()
//-------------------------------------------------------------------------------
// Get constant reference to vertex buffer of tetrahedron mesh.
// Some vertecies may be unneeded.
//-------------------------------------------------------------------------------
const std::vector<glm::vec3>& LsTetrahedronMesh::GetVertecies() {
  return vertexBuffer;
}

//-------------------------------------------------------------------------------
// @ LsTetrahedronMesh::GetIndecies()
//-------------------------------------------------------------------------------
// Get constant reference to index buffer of tetrahedron mesh.
// Every tetrahedron is valid at all times.
//-------------------------------------------------------------------------------
const std::vector<LsTetrahedronMesh::LsTetrahedron>& LsTetrahedronMesh::GetIndecies() {
  return indexBuffer;
}
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "LsTetrahedronMeshTest.h"
#include "LsBCCLatticeTests.h"
#include "LsPatternMatchersTest.h"
#include "LsIsosphereTests.h"

TEST_CASE( "aaa", "bbb" ) {
  REQUIRE ( 8 == 8 );
  //REQUIRE ( 8 == 7 );
}

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ LsAttachConsole
//-------------------------------------------------------------------------------
// Create a console and redirect all standard IO
//-------------------------------------------------------------------------------
void LsOpenConsole() {
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONOUT$", "r", stdin);
}

//-------------------------------------------------------------------------------
// @ LsAttachConsole
//-------------------------------------------------------------------------------
// Free console and close file handles
//-------------------------------------------------------------------------------
void LsCloseConsole() {
  fclose(stdout);
  fclose(stderr);
  fclose(stdin);
  FreeConsole();
}
//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <windows.h>
#include <string>

//-------------------------------------------------------------------------------
//-- Globals --------------------------------------------------------------------
//-------------------------------------------------------------------------------

bool dialogShowing;
HWND windowHandle;

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsSetDialogParentWindow(HWND handle) {
  windowHandle = handle;
}

bool LsIsDialogShowing() {
  return dialogShowing;
}

void LsAbort(std::string& msg) {
  dialogShowing = true;
  MessageBox(windowHandle,
             msg.c_str(),
             "Error",
             MB_OK | MB_ICONERROR);
  throw 1;
}

void LsAbort(const char* msg) {
  LsAbort(std::string(msg));
}

void LsAbort() {
  throw 1;
}

void LsError() {
  LsAbort(std::string("Error occurred, view log for details."));   
}

void LsExitFatal(std::string message, std::string caption) 
{
  MessageBox( NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
  throw 1;
}

void LsErrorMessage(std::string message, std::string caption) 
{
  MessageBox( NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
}

void LsMessageBox(std::string message, std::string caption) 
{
  MessageBox( NULL, message.c_str(), caption.c_str(), MB_OK );
}
//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <windows.h>

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

std::vector<char> GetBinaryFileContents( std::string const &filename ) {
  // char str[256];
  // GetCurrentDirectory(256, &str[0]);
  // throw std::string(str);
  try {
	  std::ifstream file;
	  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	  file.open(filename, std::ios::binary);

	  if (file.fail()) {
		  std::cout << "Could not open \"" << filename << "\" file!" << std::endl;
		  return std::vector<char>();
	  }

	  std::streampos begin, end;
	  begin = file.tellg();
	  file.seekg(0, std::ios::end);
	  end = file.tellg();

	  std::vector<char> result(static_cast<size_t>(end - begin));
	  file.seekg(0, std::ios::beg);
	  file.read(&result[0], end - begin);
	  file.close();

	  return result;
  }
  catch (std::ifstream::failure e) {
	  throw std::string(strerror(errno));
	  //throw std::string(e.what());
  }
}//===============================================================================
// @ LsVulkanLoader.cpp
// 
// Dynamic loader for vulkan
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#include <stdexcept>
#include <iostream>
#include <windows.h>
#include <vector>
#include "vulkan_dynamic.hpp"

//-------------------------------------------------------------------------------
//-- Globals --------------------------------------------------------------------
//-------------------------------------------------------------------------------

HMODULE LsVulkanLibrary;

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsLoadVulkanLibrary() {
    LsVulkanLibrary = LoadLibrary("vulkan-1.dll");
    if (LsVulkanLibrary == nullptr) {
        std::cout << "Could not load Vulkan library!" << std::endl;
        throw 1;
    }
}

void LsUnloadVulkanLibrary() {
    if ( LsVulkanLibrary ) {
        FreeLibrary( LsVulkanLibrary );
    }
}

void LsLoadExportedEntryPoints() {
    #define VK_EXPORTED_FUNCTION( fun )                                                  \
        if(!(fun = (PFN_##fun)GetProcAddress( LsVulkanLibrary, #fun ))) {                \
            std::cout << "Could not load exported function: ##fun!" << std::endl;        \
            throw 1;                                                                     \
        }
    #define VK_USE_CORE
    #include "LsVulkanCommands.inl"
    #undef VK_USE_CORE
}

void LsLoadGlobalLevelEntryPoints() {
    #define VK_GLOBAL_LEVEL_FUNCTION( fun )                                              \
        if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {               \
            std::cout << "Could not load global level function: ##fun!" << std::endl;    \
            throw 1;                                                                     \
        }
    #define VK_USE_CORE
    #include "LsVulkanCommands.inl"
    #undef VK_USE_CORE
}

void LsLoadInstanceLevelEntryPoints(VkInstance instance, std::vector<const char*> extensions) {
    // Core
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                            \
        if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( instance, #fun )) ) {                        \
            throw std::runtime_error("Could not load instnace level function: ##fun!");  \
            std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
            throw 1;                                                                     \
        }
    #define VK_USE_CORE
    #include "LsVulkanCommands.inl"
    #undef VK_USE_CORE

    // Extensions
    bool KHR_DISPLAY = false;
    bool KHR_SURFACE = false;
    bool KHR_WIN32_SURFACE = false;
    bool EXT_DEBUG_REPORT = false;
    bool KHR_DISPLAY_SWAPCHAIN = false;
    bool KHR_SWAPCHAIN = false;
    bool NV_EXTERNAL_MEMORY_CAPABILITIES = false;

    // Scan the list and set extension flags
    for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
    {
        if (strcmp(extensions[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
            KHR_DISPLAY = true;
        } else if (strcmp(extensions[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            KHR_SURFACE = true;
        } else if (strcmp(extensions[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            KHR_WIN32_SURFACE = true;
        } else if (strcmp(extensions[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            EXT_DEBUG_REPORT = true;
        } else if (strcmp(extensions[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_DISPLAY_SWAPCHAIN = true;
        } else if (strcmp(extensions[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_SWAPCHAIN = true;
        }
    }

    #define LOAD_FUNCTION( fun )                                                             \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( instance, #fun )) ) {                      \
                throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                throw 1;                                                                     \
            }

    #define VK_USE_KHR_DISPLAY
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_DISPLAY )                                                                   \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DISPLAY

    #define VK_USE_KHR_SURFACE
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_SURFACE )                                                                   \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_SURFACE    

    #define VK_USE_KHR_WIN32_SURFACE
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_WIN32_SURFACE )                                                             \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_WIN32_SURFACE

    #define VK_USE_EXT_DEBUG_REPORT
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( EXT_DEBUG_REPORT )                                                              \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_EXT_DEBUG_REPORT

    #define VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES
    #define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                \
        if ( NV_EXTERNAL_MEMORY_CAPABILITIES )                                               \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES

    #undef LOAD_FUNCTION
}

void LsLoadDeviceLevelEntryPoints(VkDevice device, std::vector<const char*> extensions) {
    // Core
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                              \
        if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( device, #fun )) ) {                \
            throw std::runtime_error("Could not load instnace level function: ##fun!");  \
            std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
            throw 1;                                                                     \
        }
    #define VK_USE_CORE
    #include "LsVulkanCommands.inl"
    #undef VK_USE_CORE

    // Extensions
    bool KHR_DISPLAY = false;
    bool KHR_SURFACE = false;
    bool KHR_WIN32_SURFACE = false;
    bool EXT_DEBUG_REPORT = false;
    bool KHR_DISPLAY_SWAPCHAIN = false;
    bool KHR_SWAPCHAIN = false;
    bool KHR_DEBUG_MARKER = false;
    bool AMD_DRAW_INDIRECT_COUNT = false;
    bool NV_EXTERNAL_MEMORY_WIN32 = false;

    // Scan the list and set extension flags
    for (int i = 0; i < static_cast<int>(extensions.size()); ++i)
    {
        if (strcmp(extensions[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
            KHR_DISPLAY = true;
        } else if (strcmp(extensions[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            KHR_SURFACE = true;
        } else if (strcmp(extensions[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            KHR_WIN32_SURFACE = true;
        } else if (strcmp(extensions[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            EXT_DEBUG_REPORT = true;
        } else if (strcmp(extensions[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_DISPLAY_SWAPCHAIN = true;
        } else if (strcmp(extensions[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            KHR_SWAPCHAIN = true;
        }
    }

    #define LOAD_FUNCTION( fun )                                                             \
    if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( device, #fun )) ) {                                   \
                throw std::runtime_error("Could not load instnace level function: ##fun!");  \
                std::cout << "Could not load instnace level function: ##fun!" << std::endl;  \
                throw 1;                                                                     \
            }

    // NOTE: Unfortunately the preprocessor does not support defining macros from other macros
    //       so it is not possible to move out this kind of code into a macro to avoid repetition
    #define VK_USE_KHR_DISPLAY
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_DISPLAY )                                                                 \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DISPLAY

    #define VK_USE_KHR_SURFACE
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_SURFACE )                                                                 \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_SURFACE    

    #define VK_USE_KHR_WIN32_SURFACE
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( KHR_WIN32_SURFACE )                                                           \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_WIN32_SURFACE

    #define VK_USE_EXT_DEBUG_REPORT
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
        if ( EXT_DEBUG_REPORT )                                                            \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_EXT_DEBUG_REPORT

    #define VK_USE_KHR_DISPLAY_SWAPCHAIN
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( KHR_DISPLAY_SWAPCHAIN )                                                        \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DISPLAY_SWAPCHAIN

    #define VK_USE_KHR_SWAPCHAIN
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( KHR_SWAPCHAIN )                                                                \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_SWAPCHAIN

    #define VK_USE_KHR_DEBUG_MARKER
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( KHR_DEBUG_MARKER )                                                             \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_KHR_DEBUG_MARKER

    #define VK_USE_AMD_DRAW_INDIRECT_COUNT
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( AMD_DRAW_INDIRECT_COUNT )                                                      \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_AMD_DRAW_INDIRECT_COUNT

    #define VK_USE_NV_EXTERNAL_MEMORY_WIN32
    #define VK_DEVICE_LEVEL_FUNCTION( fun )                                                \
       if ( NV_EXTERNAL_MEMORY_WIN32 )                                                     \
            LOAD_FUNCTION(fun)
    #include "LsVulkanCommands.inl"
    #undef VK_USE_NV_EXTERNAL_MEMORY_WIN32

    #undef LOAD_FUNCTION
}//===============================================================================
// @ vulkan_dynamic.cpp
// 
// Definitions for vulkan_dynamic.hpp
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#include <vulkan_dynamic.hpp>

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------
#define VK_EXPORTED_FUNCTION( fun ) PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION( fun) PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION( fun ) PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION( fun ) PFN_##fun fun;

#define VK_USE_CORE
#define VK_USE_KHR_DISPLAY
#define VK_USE_KHR_SURFACE
#define VK_USE_KHR_WIN32_SURFACE
#define VK_USE_EXT_DEBUG_REPORT
#define VK_USE_KHR_DISPLAY_SWAPCHAIN
#define VK_USE_KHR_SWAPCHAIN
#define VK_USE_KHR_DEBUG_MARKER
#define VK_USE_AMD_DRAW_INDIRECT_COUNT
#define VK_USE_NV_EXTERNAL_MEMORY_WIN32
#define VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES

#include "LsVulkanCommands.inl"

#undef VK_USE_CORE
#undef VK_USE_KHR_DISPLAY
#undef VK_USE_KHR_SURFACE
#undef VK_USE_KHR_WIN32_SURFACE
#undef VK_USE_EXT_DEBUG_REPORT
#undef VK_USE_KHR_DISPLAY_SWAPCHAIN
#undef VK_USE_KHR_SWAPCHAIN
#undef VK_USE_KHR_DEBUG_MARKER
#undef VK_USE_AMD_DRAW_INDIRECT_COUNT
#undef VK_USE_NV_EXTERNAL_MEMORY_WIN32
#undef VK_USE_NV_EXTERNAL_MEMORY_CAPABILITIES#include <windows.h>
#include <LsFWin32MessageHandler.h>
#include <vector>
#include <algorithm>

std::vector<LsFWin32MessageHandler*> LsFWin32MessageHandler::Handlers;

LsFWin32MessageHandler::LsFWin32MessageHandler() {
  Handlers.push_back(this);
}

LsFWin32MessageHandler::~LsFWin32MessageHandler() {
  Handlers.erase(std::remove(Handlers.begin(), Handlers.end(), this), Handlers.end());
}

void LsFWin32MessageHandler::BroadCastWin32Message(UINT uMsg, WPARAM wParam, LPARAM lParam){
  for (auto i = Handlers.begin(); i != Handlers.end(); ++i)
  {
    (*i)->OnWin32Message(uMsg, wParam, lParam);
  }
}#include <windows.h>
#include <assert.h>
#include <string>
#include <LsWin32MainWindow.h>
#include <LsFWin32MessageHandler.h>
#include <DWINTAB.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return LsWin32MainWindow::Get()->HandleMessage(hwnd, uMsg, wParam, lParam);
}

void LsWin32MainWindow::Create(HINSTANCE hInstance, std::string title, int x, int y, int width, int height) {
  WNDCLASSEX windowClass = {};
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  windowClass.lpfnWndProc = WndProc;
  windowClass.hInstance = hInstance;
  windowClass.lpszClassName = "LsMainWindow";
  windowClass.hCursor = NULL;
  ::RegisterClassEx(&windowClass);

  // Calculate windows size with requested client area
  RECT windowRegion;
  windowRegion.left = x;
  windowRegion.right = x + width;
  windowRegion.top = y;
  windowRegion.bottom = y + height;

  ::AdjustWindowRect(&windowRegion, WS_OVERLAPPEDWINDOW | WS_VISIBLE, false);

  windowHandle = NULL;
  HWND handle = ::CreateWindowEx( WS_EX_APPWINDOW,
                                  "LsMainWindow",
                                  title.c_str(),
                                  WS_OVERLAPPEDWINDOW, // | WS_VISIBLE,
                                  windowRegion.left,
                                  windowRegion.top,
                                  windowRegion.right - windowRegion.left,
                                  windowRegion.bottom - windowRegion.top,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  NULL );
  
  // Sanity check, make sure windowHandle was set inside WndProc to correct value
  assert(handle == windowHandle);
}

LsWin32MainWindow* LsWin32MainWindow::Get() {
  return &window;
}

HWND LsWin32MainWindow::GetWindowHandle() {
  return windowHandle;
}

void LsWin32MainWindow::WaitForMessages() {
  ::WaitMessage();
}

bool LsWin32MainWindow::ProcessMessages() {
  while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
    if ( !::GetMessage( &msg, NULL, 0, 0 ) ) {
      return false;
    }
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  return true;
}

bool LsWin32MainWindow::ProcessNextMessage() {
  if ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
    if ( !::GetMessage( &msg, NULL, 0, 0 ) ) {
      return false;
    }
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  return true;
}

void LsWin32MainWindow::Destroy() {
  DestroyWindow(windowHandle);
}

void LsWin32MainWindow::Hide() {
  ShowWindow(windowHandle, SW_HIDE);
}

void LsWin32MainWindow::Show() {
  ShowWindow(windowHandle, SW_SHOW);
}

void LsWin32MainWindow::Minimize() {
  ShowWindow(windowHandle, SW_MINIMIZE);
}

void LsWin32MainWindow::Restore() {
  ShowWindow(windowHandle, SW_RESTORE);
}

void LsWin32MainWindow::Maximize() {
  ShowWindow(windowHandle, SW_MAXIMIZE);
}

void LsWin32MainWindow::ShowMouse() {
  mouseVisible = true;
}

void LsWin32MainWindow::HideMouse() {
  mouseVisible = false;
}

void LsWin32MainWindow::GetClientArea(int* x, int* y, int* width, int* height) {
  RECT clientRect;
  ::GetClientRect(windowHandle, &clientRect);

  if (x) {
    *x = clientRect.left;
  }

  if (y) {
    *y = clientRect.top;
  }

  if (width) {
    *width = clientRect.right - clientRect.left;
  }

  if (height) {
    *height = clientRect.bottom - clientRect.top;
  }
}

POINT LsWin32MainWindow::GetMousePosition() {
  POINT cursor;
  GetCursorPos(&cursor);
  ScreenToClient(windowHandle, &cursor);
  return cursor;
}

LsWin32MainWindow LsWin32MainWindow::window;

LsWin32MainWindow::LsWin32MainWindow() {
  arrowCursor = LoadCursor(NULL, IDC_ARROW);
}

LsWin32MainWindow::~LsWin32MainWindow() {
  
}

LRESULT LsWin32MainWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  // Set window handle as soon as possible
  if ( !windowHandle ) {
     LsWin32MainWindow::Get()->windowHandle = hwnd;
  }

  LsFWin32MessageHandler::BroadCastWin32Message(uMsg, wParam, lParam);

  // Control mouse visibility in client area
  if ( uMsg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT )
  {
    if ( mouseVisible )
    {
      SetCursor(arrowCursor);
    } else {
      SetCursor(NULL);
    }
    return TRUE; // halt further processing
  }

  switch( uMsg ) {
    case WT_PACKET:
    return FALSE;
    break;
    case WM_MOUSEMOVE:
    // Set mouse event tracking to get WM_MOUSELEAVE event
    if ( !mouseInClient )
    {
      mouseInClient = TRUE;
      TRACKMOUSEEVENT trackMouseEvent = { sizeof(TRACKMOUSEEVENT) };
      trackMouseEvent.dwFlags = TME_LEAVE;
      trackMouseEvent.hwndTrack = windowHandle;
      TrackMouseEvent(&trackMouseEvent);
    }
    break;
    case WM_MOUSELEAVE:
      mouseInClient = FALSE;
      return 0;
    break;
    case WM_DESTROY:
    PostQuitMessage(0);
    break;
  }
 
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}//===============================================================================
// @ DWINTAB.cpp
// 
// Wrapper for WINTAB.h with dynamically loaded functions
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <DWINTAB.h>

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

#define WT_EXPORTED_FUNCTION( fun ) PFN_##fun fun;

#include "LsWintabFunctions.inl"//===============================================================================
// @ WintabLoader.cpp
// 
// Loader for Wintab library
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <windows.h>
#include <iostream>
#include <DWINTAB.h>

//-------------------------------------------------------------------------------
//-- Globals --------------------------------------------------------------------
//-------------------------------------------------------------------------------

static HMODULE wintabLibrary;

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

void LsLoadWintabLibrary() {
  wintabLibrary = LoadLibrary("Wintab32.dll");
  if (wintabLibrary == nullptr) {
    std::cout << "Could not load Wintab library!" << std::endl;
    throw 1;
  }
}

void LsUnloadWintabLibrary() {
  if ( wintabLibrary ) {
    FreeLibrary( wintabLibrary );
  }
}

void LsLoadWintabEntryPoints() {
  #define WT_EXPORTED_FUNCTION( fun )                                         \
    if(!(fun = (PFN_##fun)GetProcAddress( wintabLibrary, #fun ))) {           \
      std::cout << "Could not load exported function: ##fun!" << std::endl;   \
      throw 1;                                                                \
    }
  #include "LsWintabFunctions.inl"
}