//===============================================================================
// @ LsVulkanLoader.h
// 
// Dynamic loader for vulkan
//
//===============================================================================

#pragma once

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------
#include "vulkan_dynamic.hpp"
#include <vector>

void LsLoadVulkanLibrary();

void LsUnloadVulkanLibrary();

void LsLoadExportedEntryPoints();

void LsLoadGlobalLevelEntryPoints();

void LsLoadInstanceLevelEntryPoints(VkInstance instance, std::vector<const char*> extensions);

void LsLoadDeviceLevelEntryPoints(VkDevice device, std::vector<const char*> extensions);