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
#include <vulkan_dynamic.hpp>
#include <vector>

void LsLoadVulkanLibrary();

void LsUnloadVulkanLibrary();

void LsLoadExportedEntryPoints();

void LsLoadGlobalLevelEntryPoints();

void LsLoadInstanceLevelEntryPoints(vk::Instance instance, std::vector<const char*> extensions);

void LsLoadDeviceLevelEntryPoints(vk::Device device, std::vector<const char*> extensions);