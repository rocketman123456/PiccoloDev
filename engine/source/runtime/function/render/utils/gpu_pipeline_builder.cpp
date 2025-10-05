#include "runtime/function/render/utils/gpu_pipeline_builder.h"
#include "runtime/core/base/macro.h"

#include <stdexcept>

namespace Piccolo
{
    GPUPipelineBuilder::GPUPipelineBuilder(VkDevice device) : m_device(device)
    {
        reset();
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setName(const std::string& name)
    {
        m_config.name = name;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setDescription(const std::string& description)
    {
        m_config.description = description;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addShaderStage(const GPUShaderStageConfig& stage)
    {
        m_config.shader_stages.push_back(stage);
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addVertexShader(const std::string& path, const std::string& entry_point)
    {
        m_config.shader_stages.push_back({ShaderType::VertexShader, path, entry_point});
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addFragmentShader(const std::string& path, const std::string& entry_point)
    {
        m_config.shader_stages.push_back({ShaderType::FragmanetShader, path, entry_point});
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addComputeShader(const std::string& path, const std::string& entry_point)
    {
        m_config.shader_stages.push_back({ShaderType::ComputeShader, path, entry_point});
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setVertexInput(const VertexInputConfig& config)
    {
        m_config.vertex_input = config;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate)
    {
        VkVertexInputBindingDescription binding_desc{};
        binding_desc.binding = binding;
        binding_desc.stride = stride;
        binding_desc.inputRate = input_rate;
        
        m_config.vertex_input.bindings.push_back(binding_desc);
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addVertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
    {
        VkVertexInputAttributeDescription attribute_desc{};
        attribute_desc.location = location;
        attribute_desc.binding = binding;
        attribute_desc.format = format;
        attribute_desc.offset = offset;
        
        m_config.vertex_input.attributes.push_back(attribute_desc);
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setInputAssembly(const InputAssemblyConfig& config)
    {
        m_config.input_assembly = config;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setTopology(VkPrimitiveTopology topology)
    {
        m_config.input_assembly.topology = topology;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setRasterization(const RasterizationConfig& config)
    {
        m_config.rasterization = config;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setCullMode(VkCullModeFlags cull_mode)
    {
        m_config.rasterization.cull_mode = cull_mode;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setPolygonMode(VkPolygonMode polygon_mode)
    {
        m_config.rasterization.polygon_mode = polygon_mode;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setMultisample(const MultisampleConfig& config)
    {
        m_config.multisample = config;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setSampleCount(VkSampleCountFlagBits samples)
    {
        m_config.multisample.rasterization_samples = samples;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setDepthStencil(const DepthStencilConfig& config)
    {
        m_config.depth_stencil = config;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::enableDepthTest(bool enable)
    {
        m_config.depth_stencil.depth_test_enable = enable;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::enableDepthWrite(bool enable)
    {
        m_config.depth_stencil.depth_write_enable = enable;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setColorBlend(const ColorBlendConfig& config)
    {
        m_config.color_blend = config;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& attachment)
    {
        m_config.color_blend.attachments.push_back(attachment);
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addDefaultColorBlendAttachment()
    {
        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;
        
        return addColorBlendAttachment(color_blend_attachment);
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setDynamicState(const DynamicStateConfig& config)
    {
        m_config.dynamic_state = config;
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addDynamicState(VkDynamicState state)
    {
        m_config.dynamic_state.states.push_back(state);
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addDescriptorSetLayout(VkDescriptorSetLayout layout)
    {
        m_config.descriptor_set_layouts.push_back(layout);
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::addPushConstantRange(const VkPushConstantRange& range)
    {
        m_config.push_constant_ranges.push_back(range);
        return *this;
    }

    GPUPipelineBuilder& GPUPipelineBuilder::setRenderPass(VkRenderPass render_pass, uint32_t subpass)
    {
        m_config.render_pass = render_pass;
        m_config.subpass = subpass;
        return *this;
    }

    VkPipeline GPUPipelineBuilder::buildGraphicsPipeline()
    {
        if (m_config.shader_stages.empty())
        {
            LOG_ERROR("No shader stages configured for pipeline: {}", m_config.name);
            return VK_NULL_HANDLE;
        }

        if (m_config.render_pass == VK_NULL_HANDLE)
        {
            LOG_ERROR("No render pass configured for pipeline: {}", m_config.name);
            return VK_NULL_HANDLE;
        }

        createShaders();
        createPipelineLayout();

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        for (const auto& shader : m_shaders)
        {
            shader_stages.push_back(shader->getCreateInfo());
        }

        // 顶点输入状态
        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(m_config.vertex_input.bindings.size());
        vertex_input_info.pVertexBindingDescriptions = m_config.vertex_input.bindings.data();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_config.vertex_input.attributes.size());
        vertex_input_info.pVertexAttributeDescriptions = m_config.vertex_input.attributes.data();

        // 输入装配状态
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = m_config.input_assembly.topology;
        input_assembly.primitiveRestartEnable = m_config.input_assembly.primitive_restart_enable;

        // 视口状态
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        // 光栅化状态
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = m_config.rasterization.depth_clamp_enable;
        rasterizer.rasterizerDiscardEnable = m_config.rasterization.rasterizer_discard_enable;
        rasterizer.polygonMode = m_config.rasterization.polygon_mode;
        rasterizer.lineWidth = m_config.rasterization.line_width;
        rasterizer.cullMode = m_config.rasterization.cull_mode;
        rasterizer.frontFace = m_config.rasterization.front_face;
        rasterizer.depthBiasEnable = m_config.rasterization.depth_bias_enable;
        rasterizer.depthBiasConstantFactor = m_config.rasterization.depth_bias_constant_factor;
        rasterizer.depthBiasClamp = m_config.rasterization.depth_bias_clamp;
        rasterizer.depthBiasSlopeFactor = m_config.rasterization.depth_bias_slope_factor;

        // 多重采样状态
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = m_config.multisample.sample_shading_enable;
        multisampling.rasterizationSamples = m_config.multisample.rasterization_samples;
        multisampling.minSampleShading = m_config.multisample.min_sample_shading;
        multisampling.pSampleMask = m_config.multisample.sample_mask;
        multisampling.alphaToCoverageEnable = m_config.multisample.alpha_to_coverage_enable;
        multisampling.alphaToOneEnable = m_config.multisample.alpha_to_one_enable;

        // 深度模板状态
        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable = m_config.depth_stencil.depth_test_enable;
        depth_stencil.depthWriteEnable = m_config.depth_stencil.depth_write_enable;
        depth_stencil.depthCompareOp = m_config.depth_stencil.depth_compare_op;
        depth_stencil.depthBoundsTestEnable = m_config.depth_stencil.depth_bounds_test_enable;
        depth_stencil.minDepthBounds = m_config.depth_stencil.min_depth_bounds;
        depth_stencil.maxDepthBounds = m_config.depth_stencil.max_depth_bounds;
        depth_stencil.stencilTestEnable = m_config.depth_stencil.stencil_test_enable;
        depth_stencil.front = m_config.depth_stencil.front;
        depth_stencil.back = m_config.depth_stencil.back;

        // 颜色混合状态
        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = m_config.color_blend.logic_op_enable;
        color_blending.logicOp = m_config.color_blend.logic_op;
        color_blending.attachmentCount = static_cast<uint32_t>(m_config.color_blend.attachments.size());
        color_blending.pAttachments = m_config.color_blend.attachments.data();
        memcpy(color_blending.blendConstants, m_config.color_blend.blend_constants, sizeof(color_blending.blendConstants));
        
        // 如果没有颜色混合附件，创建一个默认的
        VkPipelineColorBlendAttachmentState default_color_blend_attachment{};
        if (m_config.color_blend.attachments.empty())
        {
            default_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            default_color_blend_attachment.blendEnable = VK_FALSE;
            color_blending.attachmentCount = 1;
            color_blending.pAttachments = &default_color_blend_attachment;
        }

        // 动态状态
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(m_config.dynamic_state.states.size());
        dynamic_state.pDynamicStates = m_config.dynamic_state.states.data();

        // 图形管道创建信息
        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline_info.pStages = shader_stages.data();
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = m_pipeline_layout;
        pipeline_info.renderPass = m_config.render_pass;
        pipeline_info.subpass = m_config.subpass;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info.basePipelineIndex = -1;

        VkPipeline pipeline;
        VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline);
        
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create graphics pipeline: {}", m_config.name);
            cleanupShaders();
            return VK_NULL_HANDLE;
        }

        LOG_DEBUG("Successfully created graphics pipeline: {}", m_config.name);
        cleanupShaders();
        return pipeline;
    }

    VkPipeline GPUPipelineBuilder::buildComputePipeline()
    {
        if (m_config.shader_stages.size() != 1 || m_config.shader_stages[0].type != ShaderType::ComputeShader)
        {
            LOG_ERROR("Compute pipeline requires exactly one compute shader stage");
            return VK_NULL_HANDLE;
        }

        createShaders();
        createPipelineLayout();

        VkComputePipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.stage = m_shaders[0]->getCreateInfo();
        pipeline_info.layout = m_pipeline_layout;

        VkPipeline pipeline;
        VkResult result = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline);
        
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create compute pipeline: {}", m_config.name);
            cleanupShaders();
            return VK_NULL_HANDLE;
        }

        LOG_DEBUG("Successfully created compute pipeline: {}", m_config.name);
        cleanupShaders();
        return pipeline;
    }

    void GPUPipelineBuilder::reset()
    {
        m_config = {};
        m_config.name = "Unnamed Pipeline";
        m_config.description = "No description";
        
        // 设置默认值
        m_config.input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        m_config.rasterization.polygon_mode = VK_POLYGON_MODE_FILL;
        m_config.rasterization.cull_mode = VK_CULL_MODE_BACK_BIT;
        m_config.rasterization.front_face = VK_FRONT_FACE_CLOCKWISE;
        m_config.multisample.rasterization_samples = VK_SAMPLE_COUNT_1_BIT;
        m_config.depth_stencil.depth_test_enable = true;
        m_config.depth_stencil.depth_write_enable = true;
        m_config.depth_stencil.depth_compare_op = VK_COMPARE_OP_LESS;
        m_config.dynamic_state.states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        
        cleanupShaders();
        if (m_pipeline_layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
            m_pipeline_layout = VK_NULL_HANDLE;
        }
    }

    void GPUPipelineBuilder::createShaders()
    {
        cleanupShaders();
        
        for (const auto& stage : m_config.shader_stages)
        {
            auto shader = std::make_shared<GPUShader>(m_device, stage.shader_path, stage.type, stage.entry_point);
            m_shaders.push_back(shader);
        }
    }

    void GPUPipelineBuilder::createPipelineLayout()
    {
        if (m_pipeline_layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        }

        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = static_cast<uint32_t>(m_config.descriptor_set_layouts.size());
        layout_info.pSetLayouts = m_config.descriptor_set_layouts.data();
        layout_info.pushConstantRangeCount = static_cast<uint32_t>(m_config.push_constant_ranges.size());
        layout_info.pPushConstantRanges = m_config.push_constant_ranges.data();

        VkResult result = vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_pipeline_layout);
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create pipeline layout for: {}", m_config.name);
        }
    }

    void GPUPipelineBuilder::cleanupShaders()
    {
        m_shaders.clear();
    }

    // 预定义配置工厂实现
    GPUPipelineBuilderConfig GPUPipelineConfigFactory::createBasicTrianglePipeline()
    {
        GPUPipelineBuilderConfig config;
        config.name = "Basic Triangle Pipeline";
        config.description = "Basic pipeline for rendering triangles";
        
        config.shader_stages = {
            {ShaderType::VertexShader, "asset/shader/glsl/_shader_base.vert", "main"},
            {ShaderType::FragmanetShader, "asset/shader/glsl/_shader_base.frag", "main"}
        };
        
        return config;
    }

    GPUPipelineBuilderConfig GPUPipelineConfigFactory::createDepthTestPipeline()
    {
        auto config = createBasicTrianglePipeline();
        config.name = "Depth Test Pipeline";
        config.description = "Pipeline with depth testing enabled";
        
        config.depth_stencil.depth_test_enable = true;
        config.depth_stencil.depth_write_enable = true;
        config.depth_stencil.depth_compare_op = VK_COMPARE_OP_LESS;
        
        return config;
    }

    GPUPipelineBuilderConfig GPUPipelineConfigFactory::createWireframePipeline()
    {
        auto config = createBasicTrianglePipeline();
        config.name = "Wireframe Pipeline";
        config.description = "Pipeline for wireframe rendering";
        
        config.rasterization.polygon_mode = VK_POLYGON_MODE_LINE;
        config.rasterization.line_width = 2.0f;
        
        return config;
    }

    GPUPipelineBuilderConfig GPUPipelineConfigFactory::createComputePipeline(const std::string& compute_shader_path)
    {
        GPUPipelineBuilderConfig config;
        config.name = "Compute Pipeline";
        config.description = "Compute pipeline for general purpose computing";
        
        config.shader_stages = {
            {ShaderType::ComputeShader, compute_shader_path, "main"}
        };
        
        return config;
    }

    GPUPipelineBuilderConfig GPUPipelineConfigFactory::createPostProcessPipeline()
    {
        GPUPipelineBuilderConfig config;
        config.name = "Post Process Pipeline";
        config.description = "Pipeline for post-processing effects";
        
        config.shader_stages = {
            {ShaderType::VertexShader, "asset/shader/glsl/post_process.vert", "main"},
            {ShaderType::FragmanetShader, "asset/shader/glsl/post_process.frag", "main"}
        };
        
        // 后处理通常不需要深度测试
        config.depth_stencil.depth_test_enable = false;
        config.depth_stencil.depth_write_enable = false;
        
        return config;
    }
} // namespace Piccolo
