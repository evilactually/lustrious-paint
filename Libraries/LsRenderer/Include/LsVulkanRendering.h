#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

VkRenderPass CreateSimpleRenderPass(VkDevice& device, VkFormat colorAttachmentFormat) {
  // NOTE: Setting loadOp to eClear requires later that we provide VkClearValue in VkRenderPassBeginInfo.
  VkAttachmentDescription attachment_descriptions[] = {
    { 
      0,                                       // VkAttachmentDescriptionFlags   flags
      colorAttachmentFormat,                   // VkFormat                       format
      VK_SAMPLE_COUNT_1_BIT,                   // VkSampleCountFlagBits          samples
      VK_ATTACHMENT_LOAD_OP_LOAD,              // VkAttachmentLoadOp             loadOp
      VK_ATTACHMENT_STORE_OP_STORE,            // VkAttachmentStoreOp            storeOp
	  VK_ATTACHMENT_LOAD_OP_DONT_CARE,         // VkAttachmentLoadOp             stencilLoadOp
	  VK_ATTACHMENT_STORE_OP_DONT_CARE,        // VkAttachmentStoreOp            stencilStoreOp
	  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,         // VkImageLayout                  initialLayout
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR          // VkImageLayout                  finalLayout
    }
  };

  VkAttachmentReference color_attachment_references[] = {
    {
      0,                                        // uint32_t                       attachment
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // VkImageLayout                  layout
    }
  };

  VkSubpassDescription subpass_descriptions[] = {
    {
      0,                                        // VkSubpassDescriptionFlags      flags
      VK_PIPELINE_BIND_POINT_GRAPHICS,          // VkPipelineBindPoint            pipelineBindPoint
      0,                                        // uint32_t                       inputAttachmentCount
      nullptr,                                  // const VkAttachmentReference    *pInputAttachments
      1,                                        // uint32_t                       colorAttachmentCount
      color_attachment_references,              // const VkAttachmentReference    *pColorAttachments
      nullptr,                                  // const VkAttachmentReference    *pResolveAttachments
      nullptr,                                  // const VkAttachmentReference    *pDepthStencilAttachment
      0,                                        // uint32_t                       preserveAttachmentCount
      nullptr                                   // const uint32_t*                pPreserveAttachments
    }
  };

  VkRenderPassCreateInfo render_pass_create_info = {
	VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	nullptr,
    0,                                          // VkRenderPassCreateFlags        flags
    1,                                          // uint32_t                       attachmentCount
    attachment_descriptions,                    // const VkAttachmentDescription  *pAttachments
    1,                                          // uint32_t                       subpassCount
    subpass_descriptions,                       // const VkSubpassDescription     *pSubpasses
    0,                                          // uint32_t                       dependencyCount
    nullptr                                     // const VkSubpassDependency      *pDependencies
  };

  VkRenderPass renderPass;
  if( vkCreateRenderPass( device, &render_pass_create_info, nullptr, &renderPass ) != VK_SUCCESS ) {
    throw std::string("Could not create render pass!");
  }

  return renderPass;
}

std::vector<VkFramebuffer> CreateFramebuffers(VkDevice& device,
                                              std::vector<VkImageView>& imageViews,
                                              VkExtent2D& extent,
                                              VkRenderPass& renderPass) {
  std::vector<VkFramebuffer> framebuffers;

  for (auto imageView:imageViews) {
    VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.flags = 0;
	framebuffer_create_info.renderPass = renderPass;
	framebuffer_create_info.attachmentCount = 1;
	framebuffer_create_info.pAttachments = &imageView;
	framebuffer_create_info.width = extent.width;
	framebuffer_create_info.height = extent.height;
	framebuffer_create_info.layers = 1;

    VkFramebuffer framebuffer;
    if( vkCreateFramebuffer( device, &framebuffer_create_info, nullptr, &framebuffer ) != VK_SUCCESS ) {
      throw std::string("Could not create a framebuffer!");
    }

    framebuffers.push_back(framebuffer);
  }

  return framebuffers;
}

VkPipelineLayout CreatePipelineLayout(VkDevice& device, size_t pushConstantsSize) {
  VkPushConstantRange pushConstantRange = {
    VK_SHADER_STAGE_VERTEX_BIT | 
    VK_SHADER_STAGE_FRAGMENT_BIT, // This is too specific, only for graphics pipeline
    0,
    pushConstantsSize
  };

  VkPipelineLayoutCreateInfo lineLayoutCreateInfo = {};
  lineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  lineLayoutCreateInfo.flags = 0;
  lineLayoutCreateInfo.setLayoutCount = 0;
  lineLayoutCreateInfo.pSetLayouts = nullptr;
  lineLayoutCreateInfo.pushConstantRangeCount = 1;
  lineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  
  VkPipelineLayout pipelineLayout;
  if( vkCreatePipelineLayout( device, &lineLayoutCreateInfo, nullptr, &pipelineLayout ) != VK_SUCCESS ) {
    throw std::string("Could not create pipeline layout!");
  }

  return pipelineLayout;
}

void CreatePrimitivePipelines(VkDevice const& device,
                              VkShaderModule const& lineVertexShader,
                              VkShaderModule const& lineFragmentShader,
                              VkShaderModule const& pointVertexShader,
                              VkShaderModule const& pointFragmentShader,
                              VkShaderModule const& imageVertexShader,
                              VkShaderModule const& imageFragmentShader,
                              VkPipelineLayout const& linePipelineLayout,
                              VkPipelineLayout const& pointPipelineLayout,
                              VkPipelineLayout const& imagePipelineLayout,
                              VkRenderPass const& renderPass,
                              VkExtent2D const& viewPortExtent,
                              VkPipeline* linePipeline,
                              VkPipeline* pointPipeline,
                              VkPipeline* imagePipeline) {
  std::vector<VkPipelineShaderStageCreateInfo> line_shader_stage_create_infos = {
    // Vertex shader
    {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,      // VkStructureType                                sType
	    nullptr,                                                  // const void*                                    pNext
      0,                                                        // VkPipelineShaderStageCreateFlags               flags
      VK_SHADER_STAGE_VERTEX_BIT,                               // VkShaderStageFlagBits                          stage
      lineVertexShader,                                         // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    },
    // Fragment shader
    {
	  VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
	    nullptr,                                                  // const void*                                    pNext
      0,                                                        // VkPipelineShaderStageCreateFlags               flags
      VK_SHADER_STAGE_FRAGMENT_BIT,                             // VkShaderStageFlagBits                          stage
      lineFragmentShader,                                       // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    }
  };

  std::vector<VkPipelineShaderStageCreateInfo> point_shader_stage_create_infos = {
    // Vertex shader
    {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,      // VkStructureType                                sType
      nullptr,                                                  // const void*                                    pNext
      0,                                                        // VkPipelineShaderStageCreateFlags               flags
	  VK_SHADER_STAGE_VERTEX_BIT,                                 // VkShaderStageFlagBits                          stage
      pointVertexShader,                                        // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    },
    // Fragment shader
    {
	  VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
	  nullptr,                                                    // const void*                                    pNext
      0,                                                        // VkPipelineShaderStageCreateFlags               flags
	  VK_SHADER_STAGE_FRAGMENT_BIT,                               // VkShaderStageFlagBits                          stage
      pointFragmentShader,                                      // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    }
  };

  std::vector<VkPipelineShaderStageCreateInfo> image_shader_stage_create_infos = {
    // Vertex shader
    {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,      // VkStructureType                                sType
      nullptr,                                                  // const void*                                    pNext
      0,                                                        // VkPipelineShaderStageCreateFlags               flags
    VK_SHADER_STAGE_VERTEX_BIT,                                 // VkShaderStageFlagBits                          stage
      imageVertexShader,                                        // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    },
    // Fragment shader
    {
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
    nullptr,                                                    // const void*                                    pNext
      0,                                                        // VkPipelineShaderStageCreateFlags               flags
    VK_SHADER_STAGE_FRAGMENT_BIT,                               // VkShaderStageFlagBits                          stage
      imageFragmentShader,                                      // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    }
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
  vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
  vertex_input_state_create_info.pVertexBindingDescriptions = nullptr;
  vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
  vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr;
  
  VkPipelineInputAssemblyStateCreateInfo line_input_assembly_state_create_info = {};
  line_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  line_input_assembly_state_create_info.flags = 0;
  line_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  line_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
  
  VkPipelineInputAssemblyStateCreateInfo point_input_assembly_state_create_info = {};
  point_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  point_input_assembly_state_create_info.flags = 0;
  point_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  point_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

  VkPipelineInputAssemblyStateCreateInfo image_input_assembly_state_create_info = {};
  image_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  image_input_assembly_state_create_info.flags = 0;
  image_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  image_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {
    0.0f,                                                         // float                                          x
    0.0f,                                                         // float                                          y
    static_cast<float>(viewPortExtent.width),                     // float                                          width
    static_cast<float>(viewPortExtent.height),                    // float                                          height
    0.0f,                                                         // float                                          minDepth
    1.0f                                                          // float                                          maxDepth
  };

  VkRect2D scissor = {
    {                                                             // VkOffset2D                                     offset
      0,                                                          // int32_t                                        x
      0                                                           // int32_t                                        y
    },
    {                                                             // VkExtent2D                                     extent
      static_cast<uint32_t>(viewPortExtent.width),                // uint32_t                                       width
      static_cast<uint32_t>(viewPortExtent.height)                // uint32_t                                       height
    }
  };

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
  viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_create_info.viewportCount = 1;
  viewport_state_create_info.pViewports = &viewport;
  viewport_state_create_info.scissorCount = 1;
  viewport_state_create_info.pScissors = &scissor;
  
  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
  rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization_state_create_info.depthClampEnable = VK_FALSE;
  rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
  rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_state_create_info.depthBiasEnable = VK_FALSE;
  rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
  rasterization_state_create_info.depthBiasClamp = 0.0f;
  rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
  rasterization_state_create_info.lineWidth = 1.0f;
      
  VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
  multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample_state_create_info.flags = 0;
  multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample_state_create_info.sampleShadingEnable = VK_FALSE;
  multisample_state_create_info.minSampleShading = 1.0f;
  multisample_state_create_info.pSampleMask = nullptr;
  multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
  multisample_state_create_info.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState color_blend_attachment_state =  {
    VK_FALSE,                                                   // VkBool32                                       blendEnable
    VK_BLEND_FACTOR_ONE,                                        // VkBlendFactor                                  srcColorBlendFactor
    VK_BLEND_FACTOR_ZERO,                                       // VkBlendFactor                                  dstColorBlendFactor
    VK_BLEND_OP_ADD,                                            // VkBlendOp                                      colorBlendOp
    VK_BLEND_FACTOR_ONE,                                        // VkBlendFactor                                  srcAlphaBlendFactor
	  VK_BLEND_FACTOR_ZERO,                                       // VkBlendFactor                                  dstAlphaBlendFactor
	  VK_BLEND_OP_ADD,                                            // VkBlendOp                                      alphaBlendOp
    VK_COLOR_COMPONENT_R_BIT |                                  // VkColorComponentFlags                          colorWriteMask
	  VK_COLOR_COMPONENT_G_BIT |
	  VK_COLOR_COMPONENT_B_BIT |
	  VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
  color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_state_create_info.flags = 0;
  color_blend_state_create_info.logicOpEnable = VK_FALSE;
  //color_blend_state_create_info.logicOpEnable = VK_TRUE; // validation errors, needs a feature enabled
  //color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
  color_blend_state_create_info.attachmentCount = 1;
  color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
  color_blend_state_create_info.blendConstants[0] = 0.0f;
  color_blend_state_create_info.blendConstants[1] = 0.0f;
  color_blend_state_create_info.blendConstants[2] = 0.0f;
  color_blend_state_create_info.blendConstants[3] = 0.0f;
    
  std::vector<VkDynamicState> line_dynamic_states = {
    VK_DYNAMIC_STATE_LINE_WIDTH
    //vk::DynamicState::eViewport,
    //vk::DynamicState::eScissor
  };

  VkPipelineDynamicStateCreateInfo line_dynamic_state_create_info = {};
  line_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  line_dynamic_state_create_info.flags = 0;
  line_dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(line_dynamic_states.size());
  line_dynamic_state_create_info.pDynamicStates = &line_dynamic_states[0];
  
  VkGraphicsPipelineCreateInfo line_pipeline_create_info = {};
  line_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  line_pipeline_create_info.flags = 0;
  line_pipeline_create_info.stageCount = static_cast<uint32_t>(line_shader_stage_create_infos.size());
  line_pipeline_create_info.pStages = &line_shader_stage_create_infos[0];
  line_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
  line_pipeline_create_info.pInputAssemblyState = &line_input_assembly_state_create_info;
  line_pipeline_create_info.pTessellationState = nullptr;
  line_pipeline_create_info.pViewportState = &viewport_state_create_info;
  line_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
  line_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
  line_pipeline_create_info.pDepthStencilState = nullptr;
  line_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
  line_pipeline_create_info.pDynamicState = &line_dynamic_state_create_info;
  line_pipeline_create_info.layout = linePipelineLayout;
  line_pipeline_create_info.renderPass = renderPass;
  line_pipeline_create_info.subpass = 0;
  line_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  line_pipeline_create_info.basePipelineIndex = -1;

  VkGraphicsPipelineCreateInfo point_pipeline_create_info = {};
  point_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  point_pipeline_create_info.flags = 0;
  point_pipeline_create_info.stageCount = static_cast<uint32_t>(point_shader_stage_create_infos.size());
  point_pipeline_create_info.pStages = &point_shader_stage_create_infos[0];
  point_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
  point_pipeline_create_info.pInputAssemblyState = &point_input_assembly_state_create_info;
  point_pipeline_create_info.pTessellationState = nullptr;
  point_pipeline_create_info.pViewportState = &viewport_state_create_info;
  point_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
  point_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
  point_pipeline_create_info.pDepthStencilState = nullptr;
  point_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
  point_pipeline_create_info.pDynamicState = nullptr;
  point_pipeline_create_info.layout = pointPipelineLayout;
  point_pipeline_create_info.renderPass = renderPass;
  point_pipeline_create_info.subpass = 0;
  point_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  point_pipeline_create_info.basePipelineIndex = -1;


  //VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
  // rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // rasterization_state_create_info.depthClampEnable = VK_FALSE;
  // rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
  // rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
  // rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  // rasterization_state_create_info.depthBiasEnable = VK_FALSE;
  // rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
  // rasterization_state_create_info.depthBiasClamp = 0.0f;
  // rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
  // rasterization_state_create_info.lineWidth = 1.0f;

  VkGraphicsPipelineCreateInfo image_pipeline_create_info = {};
  image_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  image_pipeline_create_info.flags = 0;
  image_pipeline_create_info.stageCount = static_cast<uint32_t>(image_shader_stage_create_infos.size());
  image_pipeline_create_info.pStages = &image_shader_stage_create_infos[0];
  image_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
  image_pipeline_create_info.pInputAssemblyState = &image_input_assembly_state_create_info;
  image_pipeline_create_info.pTessellationState = nullptr;
  image_pipeline_create_info.pViewportState = &viewport_state_create_info;
  image_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
  image_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
  image_pipeline_create_info.pDepthStencilState = nullptr;
  image_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
  image_pipeline_create_info.pDynamicState = nullptr;
  image_pipeline_create_info.layout = imagePipelineLayout;
  image_pipeline_create_info.renderPass = renderPass;
  image_pipeline_create_info.subpass = 0;
  image_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  image_pipeline_create_info.basePipelineIndex = -1;

  if( vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &line_pipeline_create_info, nullptr, linePipeline ) != VK_SUCCESS ) {
    throw std::string("Could not create line pipeline!");
  }

  if(vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &point_pipeline_create_info, nullptr, pointPipeline ) != VK_SUCCESS ) {
    throw std::string("Could not create point pipeline!");
  }

  if(vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &image_pipeline_create_info, nullptr, imagePipeline ) != VK_SUCCESS ) {
    throw std::string("Could not create image pipeline!");
  }
}