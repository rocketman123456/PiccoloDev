#pragma once

#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_shader.h"
#include "runtime/function/render/utils/gpu_pipeline_builder.h"
#include "runtime/function/render/utils/gpu_render_pass_builder.h"
#include "runtime/function/render/utils/gpu_buffer_builder.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    // 渲染资源管理器
    class GPURenderResourceManager
    {
    public:
        explicit GPURenderResourceManager(VkDevice device, VkPhysicalDevice physical_device);
        ~GPURenderResourceManager();

        // 管道管理
        VkPipeline createPipeline(const std::string& name, const GPUPipelineBuilderConfig& config, VkRenderPass render_pass);
        VkPipeline getPipeline(const std::string& name) const;
        void       destroyPipeline(const std::string& name);
        void       destroyAllPipelines();

        // 渲染通道管理
        VkRenderPass createRenderPass(const std::string& name, const GPURenderPassConfig& config);
        VkRenderPass getRenderPass(const std::string& name) const;
        void         destroyRenderPass(const std::string& name);
        void         destroyAllRenderPasses();

        // 描述符集布局管理
        VkDescriptorSetLayout createDescriptorSetLayout(const std::string& name, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        VkDescriptorSetLayout getDescriptorSetLayout(const std::string& name) const;
        void                  destroyDescriptorSetLayout(const std::string& name);
        void                  destroyAllDescriptorSetLayouts();

        // 帧缓冲管理
        VkFramebuffer
        createFramebuffer(const std::string& name, VkRenderPass render_pass, const std::vector<VkImageView>& attachments, uint32_t width, uint32_t height);
        VkFramebuffer getFramebuffer(const std::string& name) const;
        void          destroyFramebuffer(const std::string& name);
        void          destroyAllFramebuffers();

        // 缓冲区管理
        GPUBufferInfo createBuffer(const std::string& name, const GPUBufferConfig& config);
        GPUBufferInfo createBuffer(const std::string& name, const GPUBufferConfig& config, void* initial_data, size_t data_size);
        GPUBufferInfo getBuffer(const std::string& name) const;
        void          destroyBuffer(const std::string& name);
        void          destroyAllBuffers();

        // 便捷的缓冲区创建方法
        GPUBufferInfo createVertexBuffer(const std::string& name, size_t size, bool dynamic = false);
        GPUBufferInfo createIndexBuffer(const std::string& name, size_t size, bool dynamic = false);
        GPUBufferInfo createUniformBuffer(const std::string& name, size_t size, bool dynamic = true);
        GPUBufferInfo createStorageBuffer(const std::string& name, size_t size, bool dynamic = false);
        GPUBufferInfo createStagingBuffer(const std::string& name, size_t size);

        // 清理所有资源
        void clear();

        // 获取资源统计信息
        size_t getPipelineCount() const { return m_pipelines.size(); }
        size_t getRenderPassCount() const { return m_render_passes.size(); }
        size_t getDescriptorSetLayoutCount() const { return m_descriptor_set_layouts.size(); }
        size_t getFramebufferCount() const { return m_framebuffers.size(); }
        size_t getBufferCount() const { return m_buffers.size(); }

    private:
        VkDevice         m_device;
        VkPhysicalDevice m_physical_device;

        std::unordered_map<std::string, VkPipeline>            m_pipelines;
        std::unordered_map<std::string, VkRenderPass>          m_render_passes;
        std::unordered_map<std::string, VkDescriptorSetLayout> m_descriptor_set_layouts;
        std::unordered_map<std::string, VkFramebuffer>         m_framebuffers;
        std::unordered_map<std::string, GPUBufferInfo>         m_buffers;
    };
} // namespace Piccolo
