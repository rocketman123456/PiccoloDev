#include "runtime/function/render/render_pass.h"

#include "runtime/function/render/render_resource.h"

Piccolo::VisiableNodes Piccolo::RenderPass::m_visiable_nodes;

namespace Piccolo
{
    void RenderPass::initialize(const RenderPassInitInfo* /*init_info*/)
    {
        m_global_render_resource =
            &(std::static_pointer_cast<RenderResource>(m_render_resource)->m_global_render_resource);
    }
    void RenderPass::draw() {}

    void RenderPass::postInitialize() {}

    RHIRenderPass* RenderPass::getRenderPass() const { return m_framebuffer.render_pass; }

    std::vector<RHIImageView*> RenderPass::getFramebufferImageViews() const
    {
        std::vector<RHIImageView*> image_views;
        image_views.reserve(m_framebuffer.attachments.size());
        for (const auto& attach : m_framebuffer.attachments)
        {
            image_views.push_back(attach.view);
        }
        return image_views;
    }

    std::vector<RHIDescriptorSetLayout*> RenderPass::getDescriptorSetLayouts() const
    {
        std::vector<RHIDescriptorSetLayout*> layouts;
        layouts.reserve(m_descriptor_infos.size());
        for (const auto& desc : m_descriptor_infos)
        {
            layouts.push_back(desc.layout);
        }
        return layouts;
    }

    void RenderPass::createDescriptorSetLayout(uint32_t layout_index,
                                               const std::vector<RHIDescriptorSetLayoutBinding>& bindings)
    {
        if (m_descriptor_infos.size() <= layout_index)
        {
            m_descriptor_infos.resize(layout_index + 1);
        }

        RHIDescriptorSetLayoutCreateInfo create_info {};
        create_info.sType        = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.pNext        = nullptr;
        create_info.flags        = 0;
        create_info.bindingCount = static_cast<uint32_t>(bindings.size());
        create_info.pBindings    = bindings.data();

        if (RHI_SUCCESS != m_rhi->createDescriptorSetLayout(&create_info, m_descriptor_infos[layout_index].layout))
        {
            throw std::runtime_error("create descriptor set layout failed");
        }
    }

    void RenderPass::createPipelineLayout(uint32_t pipeline_index,
                                          const std::vector<RHIDescriptorSetLayout*>& set_layouts)
    {
        if (m_render_pipelines.size() <= pipeline_index)
        {
            m_render_pipelines.resize(pipeline_index + 1);
        }

        RHIPipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.sType          = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(set_layouts.size());
        pipeline_layout_create_info.pSetLayouts    = set_layouts.data();

        if (m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[pipeline_index].layout) != RHI_SUCCESS)
        {
            throw std::runtime_error("create pipeline layout failed");
        }
    }

    void RenderPass::createFullscreenTrianglePipeline(
        uint32_t                                              pipeline_index,
        RHIShader*                                            vert_shader_module,
        RHIShader*                                            frag_shader_module,
        RHIRenderPass*                                        render_pass,
        uint32_t                                              subpass_index,
        const RHIPipelineDepthStencilStateCreateInfo*         depth_stencil_override,
        const RHIPipelineColorBlendStateCreateInfo*           color_blend_override)
    {
        if (m_render_pipelines.size() <= pipeline_index)
        {
            m_render_pipelines.resize(pipeline_index + 1);
        }

        RHIPipelineShaderStageCreateInfo vert_info {};
        vert_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_info.stage  = RHI_SHADER_STAGE_VERTEX_BIT;
        vert_info.module = vert_shader_module;
        vert_info.pName  = "main";

        RHIPipelineShaderStageCreateInfo frag_info {};
        frag_info.sType  = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_info.stage  = RHI_SHADER_STAGE_FRAGMENT_BIT;
        frag_info.module = frag_shader_module;
        frag_info.pName  = "main";

        RHIPipelineShaderStageCreateInfo shader_stages[] = {vert_info, frag_info};

        RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
        vertex_input_state_create_info.sType                           = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.vertexBindingDescriptionCount   = 0;
        vertex_input_state_create_info.pVertexBindingDescriptions      = nullptr;
        vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_state_create_info.pVertexAttributeDescriptions    = nullptr;

        RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
        input_assembly_create_info.sType                  = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_create_info.topology               = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

        RHIPipelineViewportStateCreateInfo viewport_state_create_info {};
        viewport_state_create_info.sType         = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports    = m_rhi->getSwapchainInfo().viewport;
        viewport_state_create_info.scissorCount  = 1;
        viewport_state_create_info.pScissors     = m_rhi->getSwapchainInfo().scissor;

        RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
        rasterization_state_create_info.sType                   = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.depthClampEnable        = RHI_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
        rasterization_state_create_info.polygonMode             = RHI_POLYGON_MODE_FILL;
        rasterization_state_create_info.lineWidth               = 1.0f;
        rasterization_state_create_info.cullMode                = RHI_CULL_MODE_BACK_BIT;
        rasterization_state_create_info.frontFace               = RHI_FRONT_FACE_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable         = RHI_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        rasterization_state_create_info.depthBiasClamp          = 0.0f;
        rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

        RHIPipelineMultisampleStateCreateInfo multisample_state_create_info {};
        multisample_state_create_info.sType                = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.sampleShadingEnable  = RHI_FALSE;
        multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

        RHIPipelineColorBlendAttachmentState color_blend_attachment_state {};
        color_blend_attachment_state.colorWriteMask = RHI_COLOR_COMPONENT_R_BIT |
                                                     RHI_COLOR_COMPONENT_G_BIT |
                                                     RHI_COLOR_COMPONENT_B_BIT |
                                                     RHI_COLOR_COMPONENT_A_BIT;
        color_blend_attachment_state.blendEnable         = RHI_FALSE;
        color_blend_attachment_state.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
        color_blend_attachment_state.colorBlendOp        = RHI_BLEND_OP_ADD;
        color_blend_attachment_state.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
        color_blend_attachment_state.alphaBlendOp        = RHI_BLEND_OP_ADD;

        RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
        color_blend_state_create_info.sType           = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state_create_info.logicOpEnable   = RHI_FALSE;
        color_blend_state_create_info.logicOp         = RHI_LOGIC_OP_COPY;
        color_blend_state_create_info.attachmentCount = 1;
        color_blend_state_create_info.pAttachments    = &color_blend_attachment_state;
        color_blend_state_create_info.blendConstants[0] = 0.0f;
        color_blend_state_create_info.blendConstants[1] = 0.0f;
        color_blend_state_create_info.blendConstants[2] = 0.0f;
        color_blend_state_create_info.blendConstants[3] = 0.0f;

        RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
        depth_stencil_create_info.sType                 = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_create_info.depthTestEnable       = RHI_TRUE;
        depth_stencil_create_info.depthWriteEnable      = RHI_TRUE;
        depth_stencil_create_info.depthCompareOp        = RHI_COMPARE_OP_LESS;
        depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
        depth_stencil_create_info.stencilTestEnable     = RHI_FALSE;

        const RHIPipelineDepthStencilStateCreateInfo* depth_ptr = depth_stencil_override ? depth_stencil_override : &depth_stencil_create_info;
        const RHIPipelineColorBlendStateCreateInfo*   blend_ptr = color_blend_override   ? color_blend_override   : &color_blend_state_create_info;

        RHIDynamicState dynamic_states[] = {RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR};
        RHIPipelineDynamicStateCreateInfo dynamic_state_create_info {};
        dynamic_state_create_info.sType             = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.dynamicStateCount = 2;
        dynamic_state_create_info.pDynamicStates    = dynamic_states;

        RHIGraphicsPipelineCreateInfo pipeline_info {};
        pipeline_info.sType               = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount          = 2;
        pipeline_info.pStages             = shader_stages;
        pipeline_info.pVertexInputState   = &vertex_input_state_create_info;
        pipeline_info.pInputAssemblyState = &input_assembly_create_info;
        pipeline_info.pViewportState      = &viewport_state_create_info;
        pipeline_info.pRasterizationState = &rasterization_state_create_info;
        pipeline_info.pMultisampleState   = &multisample_state_create_info;
        pipeline_info.pColorBlendState    = blend_ptr;
        pipeline_info.pDepthStencilState  = depth_ptr;
        pipeline_info.layout              = m_render_pipelines[pipeline_index].layout;
        pipeline_info.renderPass          = render_pass;
        pipeline_info.subpass             = subpass_index;
        pipeline_info.basePipelineHandle  = RHI_NULL_HANDLE;
        pipeline_info.pDynamicState       = &dynamic_state_create_info;

        if (RHI_SUCCESS != m_rhi->createGraphicsPipelines(RHI_NULL_HANDLE, 1, &pipeline_info, m_render_pipelines[pipeline_index].pipeline))
        {
            throw std::runtime_error("create graphics pipeline failed");
        }
    }

    void RenderPass::allocateDescriptorSet(uint32_t layout_index)
    {
        if (m_descriptor_infos.size() <= layout_index)
        {
            throw std::runtime_error("descriptor set layout index out of range");
        }

        RHIDescriptorSetAllocateInfo alloc_info {};
        alloc_info.sType              = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext              = nullptr;
        alloc_info.descriptorPool     = m_rhi->getDescriptorPoor();
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts        = &m_descriptor_infos[layout_index].layout;

        if (RHI_SUCCESS != m_rhi->allocateDescriptorSets(&alloc_info, m_descriptor_infos[layout_index].descriptor_set))
        {
            throw std::runtime_error("allocate descriptor set failed");
        }
    }
} // namespace Piccolo
