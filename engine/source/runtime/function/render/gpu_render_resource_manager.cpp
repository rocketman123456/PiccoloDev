#include "runtime/function/render/gpu_render_resource_manager.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/log/log_system.h"

#include <stdexcept>

namespace Piccolo
{
    GPURenderResourceManager::GPURenderResourceManager(VkDevice device)
        : m_device(device)
    {
        LOG_INFO("GPURenderResourceManager initialized");
    }

    GPURenderResourceManager::~GPURenderResourceManager()
    {
        clear();
        LOG_INFO("GPURenderResourceManager destroyed");
    }

    VkPipeline GPURenderResourceManager::createPipeline(const std::string& name, const GPUPipelineBuilderConfig& config, VkRenderPass render_pass)
    {
        // 检查是否已存在
        if (m_pipelines.find(name) != m_pipelines.end())
        {
            LOG_WARN("Pipeline '{}' already exists, destroying old one", name);
            destroyPipeline(name);
        }

        // 使用构建器创建管道
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

        VkPipeline pipeline = builder.buildGraphicsPipeline();
        m_pipelines[name]   = pipeline;

        LOG_INFO("Created pipeline: {}", name);
        return pipeline;
    }

    VkPipeline GPURenderResourceManager::getPipeline(const std::string& name) const
    {
        auto it = m_pipelines.find(name);
        if (it != m_pipelines.end())
        {
            return it->second;
        }
        return VK_NULL_HANDLE;
    }

    void GPURenderResourceManager::destroyPipeline(const std::string& name)
    {
        auto it = m_pipelines.find(name);
        if (it != m_pipelines.end())
        {
            vkDestroyPipeline(m_device, it->second, nullptr);
            m_pipelines.erase(it);
            LOG_INFO("Destroyed pipeline: {}", name);
        }
    }

    void GPURenderResourceManager::destroyAllPipelines()
    {
        for (auto& [name, pipeline] : m_pipelines)
        {
            vkDestroyPipeline(m_device, pipeline, nullptr);
        }
        m_pipelines.clear();
        LOG_INFO("Destroyed all pipelines");
    }

    VkRenderPass GPURenderResourceManager::createRenderPass(const std::string& name, const GPURenderPassConfig& config)
    {
        // 检查是否已存在
        if (m_render_passes.find(name) != m_render_passes.end())
        {
            LOG_WARN("Render pass '{}' already exists, destroying old one", name);
            destroyRenderPass(name);
        }

        // 使用构建器创建渲染通道
        GPURenderPassBuilder builder(m_device);

        // 应用配置
        builder.setName(config.name).setDescription(config.description);

        for (const auto& attachment : config.attachments)
        {
            builder.addColorAttachment(attachment);
        }

        for (const auto& subpass : config.subpasses)
        {
            builder.addSubpass(subpass);
        }

        for (const auto& dependency : config.dependencies)
        {
            builder.addDependency(dependency);
        }

        VkRenderPass render_pass = builder.build();
        m_render_passes[name]    = render_pass;

        LOG_INFO("Created render pass: {}", name);
        return render_pass;
    }

    VkRenderPass GPURenderResourceManager::getRenderPass(const std::string& name) const
    {
        auto it = m_render_passes.find(name);
        if (it != m_render_passes.end())
        {
            return it->second;
        }
        return VK_NULL_HANDLE;
    }

    void GPURenderResourceManager::destroyRenderPass(const std::string& name)
    {
        auto it = m_render_passes.find(name);
        if (it != m_render_passes.end())
        {
            vkDestroyRenderPass(m_device, it->second, nullptr);
            m_render_passes.erase(it);
            LOG_INFO("Destroyed render pass: {}", name);
        }
    }

    void GPURenderResourceManager::destroyAllRenderPasses()
    {
        for (auto& [name, render_pass] : m_render_passes)
        {
            vkDestroyRenderPass(m_device, render_pass, nullptr);
        }
        m_render_passes.clear();
        LOG_INFO("Destroyed all render passes");
    }

    VkDescriptorSetLayout
    GPURenderResourceManager::createDescriptorSetLayout(const std::string& name, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    {
        // 检查是否已存在
        if (m_descriptor_set_layouts.find(name) != m_descriptor_set_layouts.end())
        {
            LOG_WARN("Descriptor set layout '{}' already exists, destroying old one", name);
            destroyDescriptorSetLayout(name);
        }

        VkDescriptorSetLayoutCreateInfo layout_info {};
        layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings    = bindings.data();

        VkDescriptorSetLayout layout;
        if (vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &layout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor set layout: " + name);
        }

        m_descriptor_set_layouts[name] = layout;
        LOG_INFO("Created descriptor set layout: {}", name);
        return layout;
    }

    VkDescriptorSetLayout GPURenderResourceManager::getDescriptorSetLayout(const std::string& name) const
    {
        auto it = m_descriptor_set_layouts.find(name);
        if (it != m_descriptor_set_layouts.end())
        {
            return it->second;
        }
        return VK_NULL_HANDLE;
    }

    void GPURenderResourceManager::destroyDescriptorSetLayout(const std::string& name)
    {
        auto it = m_descriptor_set_layouts.find(name);
        if (it != m_descriptor_set_layouts.end())
        {
            vkDestroyDescriptorSetLayout(m_device, it->second, nullptr);
            m_descriptor_set_layouts.erase(it);
            LOG_INFO("Destroyed descriptor set layout: {}", name);
        }
    }

    void GPURenderResourceManager::destroyAllDescriptorSetLayouts()
    {
        for (auto& [name, layout] : m_descriptor_set_layouts)
        {
            vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
        }
        m_descriptor_set_layouts.clear();
        LOG_INFO("Destroyed all descriptor set layouts");
    }

    VkFramebuffer GPURenderResourceManager::createFramebuffer(
        const std::string&              name,
        VkRenderPass                    render_pass,
        const std::vector<VkImageView>& attachments,
        uint32_t                        width,
        uint32_t                        height
    )
    {
        // 检查是否已存在
        if (m_framebuffers.find(name) != m_framebuffers.end())
        {
            LOG_WARN("Framebuffer '{}' already exists, destroying old one", name);
            destroyFramebuffer(name);
        }

        VkFramebufferCreateInfo framebuffer_info {};
        framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass      = render_pass;
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments    = attachments.data();
        framebuffer_info.width           = width;
        framebuffer_info.height          = height;
        framebuffer_info.layers          = 1;

        VkFramebuffer framebuffer;
        if (vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &framebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer: " + name);
        }

        m_framebuffers[name] = framebuffer;
        LOG_INFO("Created framebuffer: {}", name);
        return framebuffer;
    }

    VkFramebuffer GPURenderResourceManager::getFramebuffer(const std::string& name) const
    {
        auto it = m_framebuffers.find(name);
        if (it != m_framebuffers.end())
        {
            return it->second;
        }
        return VK_NULL_HANDLE;
    }

    void GPURenderResourceManager::destroyFramebuffer(const std::string& name)
    {
        auto it = m_framebuffers.find(name);
        if (it != m_framebuffers.end())
        {
            vkDestroyFramebuffer(m_device, it->second, nullptr);
            m_framebuffers.erase(it);
            LOG_INFO("Destroyed framebuffer: {}", name);
        }
    }

    void GPURenderResourceManager::destroyAllFramebuffers()
    {
        for (auto& [name, framebuffer] : m_framebuffers)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        m_framebuffers.clear();
        LOG_INFO("Destroyed all framebuffers");
    }

    void GPURenderResourceManager::clear()
    {
        destroyAllFramebuffers();
        destroyAllDescriptorSetLayouts();
        destroyAllRenderPasses();
        destroyAllPipelines();
    }
} // namespace Piccolo
