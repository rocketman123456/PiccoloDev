#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_render_pass.h"
#include "runtime/function/render/gpu_render_resource.h"
#include "runtime/function/render/gpu_swap_chain.h"
#include "runtime/function/render/utils/gpu_pipeline_builder.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Piccolo
{
    // 使用新的构建器创建管道
    GPUPipeline::GPUPipeline(VkDevice device, const GPUPipelineBuilderConfig& config, VkRenderPass render_pass)
    {
        m_device = device;
        createPipelineWithBuilder(config, render_pass);
        createFramebuffers();
    }

    // 保持向后兼容的构造函数
    GPUPipeline::GPUPipeline(VkDevice device, const GPUPipelineConfig& config)
    {
        m_device = device;

        for (const auto& shader_stage : config.shader_stages)
        {
            m_shader_paths.push_back(shader_stage.shader_path);
            m_types.push_back(shader_stage.type);
            m_entry_points.push_back(shader_stage.entry_point);
        }

        createShaders();
        createGraphicsPipeline();
        createFramebuffers();
    }

    GPUPipeline::~GPUPipeline()
    {
        for (auto& framebuffer : m_swap_chain_framebuffers)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
    }

    void GPUPipeline::createShaders()
    {
        for (int i = 0; i < m_shader_paths.size(); ++i)
        {
            auto& shader_path = m_shader_paths[i];
            auto& type        = m_types[i];
            auto& entry_point = m_entry_points[i];

            auto shader = std::make_shared<GPUShader>(m_device, shader_path, type, entry_point);
            m_shaders.push_back(shader);
        }
    }

    void GPUPipeline::createGraphicsPipeline()
    {
        std::vector<VkPipelineShaderStageCreateInfo> infos;

        for (int i = 0; i < m_shader_paths.size(); ++i)
        {
            VkShaderModule module = m_shaders[i]->getShaderModule();

            VkPipelineShaderStageCreateInfo info = m_shaders[i]->getCreateInfo();

            infos.push_back(info);
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_info {};
        vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount   = 0;
        vertex_input_info.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo input_assembly {};
        input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state {};
        viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount  = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer {};
        rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth               = 1.0f;
        rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable         = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling {};
        multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable  = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment {};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable    = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blending {};
        color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable     = VK_FALSE;
        color_blending.logicOp           = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount   = 1;
        color_blending.pAttachments      = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };
        VkPipelineDynamicStateCreateInfo dynamic_state {};
        dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates    = dynamic_states.data();

        VkPipelineLayoutCreateInfo pipeline_layout_nfo {};
        pipeline_layout_nfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_nfo.setLayoutCount         = 0;
        pipeline_layout_nfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(m_device, &pipeline_layout_nfo, nullptr, &m_pipeline_layout) != VK_SUCCESS)
        {
            LOG_ERROR("failed to create pipeline layout!");
        }
        else
        {
            LOG_DEBUG("create simple pipeline layout")
        }

        auto render_pass = g_runtime_global_context.m_render_system->getRenderPass()->getRenderPass();

        VkGraphicsPipelineCreateInfo pipeline_info {};
        pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount          = static_cast<uint32_t>(infos.size());
        pipeline_info.pStages             = infos.data();
        pipeline_info.pVertexInputState   = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState      = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState   = &multisampling;
        pipeline_info.pColorBlendState    = &color_blending;
        pipeline_info.pDynamicState       = &dynamic_state;
        pipeline_info.layout              = m_pipeline_layout;
        pipeline_info.renderPass          = render_pass;
        pipeline_info.subpass             = 0;
        pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline) != VK_SUCCESS)
        {
            LOG_ERROR("failed to create graphics pipeline!");
        }
        else
        {
            LOG_DEBUG("create simple graphics pipeline")
        }

        m_shaders.clear();
    }

    void GPUPipeline::createFramebuffers()
    {
        auto& image_views = g_runtime_global_context.m_render_system->getSwapChain()->getImageViews();
        auto  extent      = g_runtime_global_context.m_render_system->getSwapChain()->getExtent();
        auto  render_pass = g_runtime_global_context.m_render_system->getRenderPass()->getRenderPass();

        m_swap_chain_framebuffers.resize(image_views.size());
        for (size_t i = 0; i < image_views.size(); i++)
        {
            VkImageView attachments[] = {image_views[i]};

            VkFramebufferCreateInfo framebufferInfo {};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = render_pass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments    = attachments;
            framebufferInfo.width           = extent.width;
            framebufferInfo.height          = extent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swap_chain_framebuffers[i]) != VK_SUCCESS)
            {
                LOG_ERROR("failed to create framebuffer!");
            }
        }
    }

    void GPUPipeline::destroyFramebuffers()
    {
        for (auto& framebuffer : m_swap_chain_framebuffers)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }

    void GPUPipeline::createPipelineWithBuilder(const GPUPipelineBuilderConfig& config, VkRenderPass render_pass)
    {
        GPUPipelineBuilder builder(m_device);

        // 应用配置
        builder.setName(config.name).setDescription(config.description).setRenderPass(render_pass);

        for (const auto& stage : config.shader_stages)
        {
            builder.addShaderStage(stage);
        }

        builder.setVertexInput(config.vertex_input)
            .setInputAssembly(config.input_assembly)
            .setRasterization(config.rasterization)
            .setMultisample(config.multisample)
            .setDepthStencil(config.depth_stencil)
            .setColorBlend(config.color_blend)
            .setDynamicState(config.dynamic_state);

        for (const auto& layout : config.descriptor_set_layouts)
        {
            builder.addDescriptorSetLayout(layout);
        }

        for (const auto& range : config.push_constant_ranges)
        {
            builder.addPushConstantRange(range);
        }

        m_pipeline        = builder.buildGraphicsPipeline();
        m_pipeline_layout = builder.getPipelineLayout();
    }

} // namespace Piccolo
