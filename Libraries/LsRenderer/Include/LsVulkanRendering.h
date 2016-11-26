#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <vulkan_dynamic.hpp>

vk::RenderPass CreateSimpleRenderPass(vk::Device& device, vk::Format colorAttachmentFormat) {
  // NOTE: Setting loadOp to eClear requires later that we provide VkClearValue in VkRenderPassBeginInfo.
  vk::AttachmentDescription attachment_descriptions[] = {
    { 
      vk::AttachmentDescriptionFlags(),        // VkAttachmentDescriptionFlags   flags
      colorAttachmentFormat,                   // VkFormat                       format
      vk::SampleCountFlagBits::e1,             // VkSampleCountFlagBits          samples
      vk::AttachmentLoadOp::eLoad,             // VkAttachmentLoadOp             loadOp
      vk::AttachmentStoreOp::eStore,           // VkAttachmentStoreOp            storeOp
      vk::AttachmentLoadOp::eDontCare,         // VkAttachmentLoadOp             stencilLoadOp
      vk::AttachmentStoreOp::eDontCare,        // VkAttachmentStoreOp            stencilStoreOp
      vk::ImageLayout::ePresentSrcKHR,         // VkImageLayout                  initialLayout
      vk::ImageLayout::ePresentSrcKHR          // VkImageLayout                  finalLayout
    }
  };

  vk::AttachmentReference color_attachment_references[] = {
    {
      0,                                        // uint32_t                       attachment
      vk::ImageLayout::eColorAttachmentOptimal, // VkImageLayout                  layout
    }
  };

  vk::SubpassDescription subpass_descriptions[] = {
    {
      vk::SubpassDescriptionFlags(),            // VkSubpassDescriptionFlags      flags
      vk::PipelineBindPoint::eGraphics,         // VkPipelineBindPoint            pipelineBindPoint
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

  vk::RenderPassCreateInfo render_pass_create_info = {
    vk::RenderPassCreateFlags(),                // VkRenderPassCreateFlags        flags
    1,                                          // uint32_t                       attachmentCount
    attachment_descriptions,                    // const VkAttachmentDescription  *pAttachments
    1,                                          // uint32_t                       subpassCount
    subpass_descriptions,                       // const VkSubpassDescription     *pSubpasses
    0,                                          // uint32_t                       dependencyCount
    nullptr                                     // const VkSubpassDependency      *pDependencies
  };

  vk::RenderPass renderPass;
  if( device.createRenderPass( &render_pass_create_info, nullptr, &renderPass ) != vk::Result::eSuccess ) {
    throw std::string("Could not create render pass!");
  }

  return renderPass;
}

std::vector<vk::Framebuffer> CreateFramebuffers(vk::Device& device,
                                                std::vector<vk::ImageView>& imageViews,
                                                vk::Extent2D& extent,
                                                vk::RenderPass& renderPass) {
  std::vector<vk::Framebuffer> framebuffers;

  for (auto imageView:imageViews) {
    vk::FramebufferCreateInfo framebuffer_create_info(
      vk::FramebufferCreateFlags(),                                // VkFramebufferCreateFlags       flags
      renderPass,                                                  // VkRenderPass                   renderPass
      1, // same as number of attachments from CreateRenderPass    // uint32_t                       attachmentCount
      &imageView,                                                  // const VkImageView              *pAttachments
      extent.width,                                                // uint32_t                       width
      extent.height,                                               // uint32_t                       height
      1);                                                          // uint32_t                       layers

    vk::Framebuffer framebuffer;
    if( device.createFramebuffer( &framebuffer_create_info, nullptr, &framebuffer ) != vk::Result::eSuccess ) {
      throw std::string("Could not create a framebuffer!");
    }

    framebuffers.push_back(framebuffer);
  }

  return framebuffers;
}

vk::PipelineLayout CreatePipelineLayout(vk::Device& device, size_t pushConstantsSize) {
  vk::PushConstantRange pushConstantRange = {
    vk::ShaderStageFlagBits::eVertex | 
    vk::ShaderStageFlagBits::eFragment,
    0,
    pushConstantsSize
  };

  vk::PipelineLayoutCreateInfo lineLayoutCreateInfo = {
    vk::PipelineLayoutCreateFlags(), // VkPipelineLayoutCreateFlags    flags
    0,                               // uint32_t                       setLayoutCount
    nullptr,                         // const VkDescriptorSetLayout    *pSetLayouts
    1,                               // uint32_t                       pushConstantRangeCount
    &pushConstantRange               // const VkPushConstantRange      *pPushConstantRanges
  };

  vk::PipelineLayout pipelineLayout;
  if( device.createPipelineLayout( &lineLayoutCreateInfo, nullptr, &pipelineLayout ) != vk::Result::eSuccess ) {
    throw std::string("Could not create pipeline layout!");
  }

  return pipelineLayout;
}

void CreatePrimitivePipelines(vk::Device const& device,
                              vk::ShaderModule const& lineVertexShader,
                              vk::ShaderModule const& lineFragmentShader,
                              vk::ShaderModule const& pointVertexShader,
                              vk::ShaderModule const& pointFragmentShader,
                              vk::PipelineLayout const& linePipelineLayout,
                              vk::PipelineLayout const& pointPipelineLayout,
                              vk::RenderPass const& renderPass,
                              vk::Extent2D const& viewPortExtent,
                              vk::Pipeline* linePipeline,
                              vk::Pipeline* pointPipeline) {
  std::vector<vk::PipelineShaderStageCreateInfo> line_shader_stage_create_infos = {
    // Vertex shader
    {
      vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
      vk::ShaderStageFlagBits::eVertex,                         // VkShaderStageFlagBits                          stage
      lineVertexShader,                                         // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    },
    // Fragment shader
    {
      vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
      vk::ShaderStageFlagBits::eFragment,                       // VkShaderStageFlagBits                          stage
      lineFragmentShader,                                       // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    }
  };

  std::vector<vk::PipelineShaderStageCreateInfo> point_shader_stage_create_infos = {
    // Vertex shader
    {
      vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
      vk::ShaderStageFlagBits::eVertex,                         // VkShaderStageFlagBits                          stage
      pointVertexShader,                                        // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    },
    // Fragment shader
    {
      vk::PipelineShaderStageCreateFlags(),                     // VkPipelineShaderStageCreateFlags               flags
      vk::ShaderStageFlagBits::eFragment,                       // VkShaderStageFlagBits                          stage
      pointFragmentShader,                                      // VkShaderModule                                 module
      "main",                                                   // const char                                    *pName
      nullptr                                                   // const VkSpecializationInfo                    *pSpecializationInfo
    }
  };

  vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info(
    vk::PipelineVertexInputStateCreateFlags(),       // VkPipelineVertexInputStateCreateFlags          flags;
    0,                                               // uint32_t                                       vertexBindingDescriptionCount
    nullptr,                                         // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
    0,                                               // uint32_t                                       vertexAttributeDescriptionCount
    nullptr                                          // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
    );

  vk::PipelineInputAssemblyStateCreateInfo line_input_assembly_state_create_info = {
    vk::PipelineInputAssemblyStateCreateFlags(),                  // VkPipelineInputAssemblyStateCreateFlags        flags
    vk::PrimitiveTopology::eLineList,                             // VkPrimitiveTopology                            topology
    VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
  };

  vk::PipelineInputAssemblyStateCreateInfo point_input_assembly_state_create_info = {
    vk::PipelineInputAssemblyStateCreateFlags(),                  // VkPipelineInputAssemblyStateCreateFlags        flags
    vk::PrimitiveTopology::ePointList,                            // VkPrimitiveTopology                            topology
    VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
  };

  vk::Viewport viewport = {
    0.0f,                                                         // float                                          x
    0.0f,                                                         // float                                          y
    static_cast<float>(viewPortExtent.width),                     // float                                          width
    static_cast<float>(viewPortExtent.height),                    // float                                          height
    0.0f,                                                         // float                                          minDepth
    1.0f                                                          // float                                          maxDepth
  };

  vk::Rect2D scissor = {
    {                                                             // VkOffset2D                                     offset
      0,                                                          // int32_t                                        x
      0                                                           // int32_t                                        y
    },
    {                                                             // VkExtent2D                                     extent
      static_cast<uint32_t>(viewPortExtent.width),                // uint32_t                                       width
      static_cast<uint32_t>(viewPortExtent.height)                // uint32_t                                       height
    }
  };

  vk::PipelineViewportStateCreateInfo viewport_state_create_info = {
    vk::PipelineViewportStateCreateFlags(),                       // VkPipelineViewportStateCreateFlags             flags
    1,                                                            // uint32_t                                       viewportCount
    &viewport,                                                    // const VkViewport                              *pViewports
    1,                                                            // uint32_t                                       scissorCount
    &scissor                                                      // const VkRect2D                                *pScissors
  };

  vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info = {
    vk::PipelineRasterizationStateCreateFlags(),                  // VkPipelineRasterizationStateCreateFlags        flags
    VK_FALSE,                                                     // VkBool32                                       depthClampEnable
    VK_FALSE,                                                     // VkBool32                                       rasterizerDiscardEnable
    vk::PolygonMode::eFill,                                       // VkPolygonMode                                  polygonMode
    vk::CullModeFlagBits::eBack,                                  // VkCullModeFlags                                cullMode
    vk::FrontFace::eCounterClockwise,                             // VkFrontFace                                    frontFace
    VK_FALSE,                                                     // VkBool32                                       depthBiasEnable
    0.0f,                                                         // float                                          depthBiasConstantFactor
    0.0f,                                                         // float                                          depthBiasClamp
    0.0f,                                                         // float                                          depthBiasSlopeFactor
    1.0f                                                          // float                                          lineWidth
  };

  vk::PipelineMultisampleStateCreateInfo multisample_state_create_info = {
    vk::PipelineMultisampleStateCreateFlags(),                    // VkPipelineMultisampleStateCreateFlags          flags
    vk::SampleCountFlagBits::e1,                                  // VkSampleCountFlagBits                          rasterizationSamples
    VK_FALSE,                                                     // VkBool32                                       sampleShadingEnable
    1.0f,                                                         // float                                          minSampleShading
    nullptr,                                                      // const VkSampleMask                            *pSampleMask
    VK_FALSE,                                                     // VkBool32                                       alphaToCoverageEnable
    VK_FALSE                                                      // VkBool32                                       alphaToOneEnable
  };

  vk::PipelineColorBlendAttachmentState color_blend_attachment_state = {
    VK_FALSE,                                                     // VkBool32                                       blendEnable
    vk::BlendFactor::eOne,                                        // VkBlendFactor                                  srcColorBlendFactor
    vk::BlendFactor::eZero,                                       // VkBlendFactor                                  dstColorBlendFactor
    vk::BlendOp::eAdd,                                            // VkBlendOp                                      colorBlendOp
    vk::BlendFactor::eOne,                                        // VkBlendFactor                                  srcAlphaBlendFactor
    vk::BlendFactor::eZero,                                       // VkBlendFactor                                  dstAlphaBlendFactor
    vk::BlendOp::eAdd,                                            // VkBlendOp                                      alphaBlendOp
    vk::ColorComponentFlagBits::eR |                              // VkColorComponentFlags                          colorWriteMask
    vk::ColorComponentFlagBits::eG |
    vk::ColorComponentFlagBits::eB |
    vk::ColorComponentFlagBits::eA
  };

  vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info = {
    vk::PipelineColorBlendStateCreateFlags(),                     // VkPipelineColorBlendStateCreateFlags           flags
    VK_FALSE,                                                     // VkBool32                                       logicOpEnable
    vk::LogicOp::eCopy,                                           // VkLogicOp                                      logicOp
    1,                                                            // uint32_t                                       attachmentCount
    &color_blend_attachment_state,                                // const VkPipelineColorBlendAttachmentState     *pAttachments
    { 0.0f, 0.0f, 0.0f, 0.0f }                                    // float                                          blendConstants[4]
  };

  std::vector<vk::DynamicState> line_dynamic_states = {
    vk::DynamicState::eLineWidth
    //vk::DynamicState::eViewport,
    //vk::DynamicState::eScissor
  };

  vk::PipelineDynamicStateCreateInfo line_dynamic_state_create_info = {
    vk::PipelineDynamicStateCreateFlags(),                        // VkPipelineDynamicStateCreateFlags              flags
    static_cast<uint32_t>(line_dynamic_states.size()),            // uint32_t                                       dynamicStateCount
    &line_dynamic_states[0]                                       // const VkDynamicState                          *pDynamicStates
  };

  vk::GraphicsPipelineCreateInfo line_pipeline_create_info(
    vk::PipelineCreateFlags(),                                    // VkPipelineCreateFlags                          flags
    static_cast<uint32_t>(line_shader_stage_create_infos.size()), // uint32_t                                       stageCount
    &line_shader_stage_create_infos[0],                           // const VkPipelineShaderStageCreateInfo         *pStages
    &vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
    &line_input_assembly_state_create_info,                       // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
    nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
    &viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
    &rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
    &multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
    nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
    &color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
    &line_dynamic_state_create_info,                              // const VkPipelineDynamicStateCreateInfo        *pDynamicState
    linePipelineLayout,                                           // VkPipelineLayout                               layout
    renderPass,                                                   // VkRenderPass                                   renderPass
    0,                                                            // uint32_t                                       subpass
    vk::Pipeline(),                                               // VkPipeline                                     basePipelineHandle
    -1                                                            // int32_t                                        basePipelineIndex
    );

  vk::GraphicsPipelineCreateInfo point_pipeline_create_info(
    vk::PipelineCreateFlags(),                                    // VkPipelineCreateFlags                          flags
    static_cast<uint32_t>(point_shader_stage_create_infos.size()),// uint32_t                                       stageCount
    &point_shader_stage_create_infos[0],                          // const VkPipelineShaderStageCreateInfo         *pStages
    &vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
    &point_input_assembly_state_create_info,                      // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
    nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
    &viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
    &rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
    &multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
    nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
    &color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
    nullptr,                                                      // const VkPipelineDynamicStateCreateInfo        *pDynamicState
    pointPipelineLayout,                                          // VkPipelineLayout                               layout
    renderPass,                                                   // VkRenderPass                                   renderPass
    0,                                                            // uint32_t                                       subpass
    vk::Pipeline(),                                               // VkPipeline                                     basePipelineHandle
    -1                                                            // int32_t                                        basePipelineIndex
    );

  if( device.createGraphicsPipelines( vk::PipelineCache(), 1, &line_pipeline_create_info, nullptr, linePipeline ) != vk::Result::eSuccess ) {
    throw std::string("Could not create line pipeline!");
  }

  if( device.createGraphicsPipelines( vk::PipelineCache(), 1, &point_pipeline_create_info, nullptr, pointPipeline ) != vk::Result::eSuccess ) {
    throw std::string("Could not create point pipeline!");
  }
}