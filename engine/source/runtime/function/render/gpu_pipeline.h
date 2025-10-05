#pragma once
#include "runtime/function/render/gpu_shader.h"
#include "runtime/function/render/utils/gpu_utils.h"
#include "runtime/function/render/utils/gpu_pipeline_builder.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <vector>

namespace Piccolo
{
    // 前向声明，完整定义在gpu_pipeline_builder.h中
    struct GPUShaderStageConfig;

    // 管道配置结构 (保持向后兼容)
    struct GPUPipelineConfig
    {
        std::string name;
        std::string description;

        std::vector<GPUShaderStageConfig> shader_stages;
    };

    class GPUPipeline
    {
    public:
        // 使用新的构建器创建管道
        GPUPipeline(VkDevice device, const GPUPipelineBuilderConfig& config, VkRenderPass render_pass);
        
        // 保持向后兼容的构造函数
        GPUPipeline(VkDevice device, const GPUPipelineConfig& config);
        
        ~GPUPipeline();

        VkPipeline                 getPipeline() const { return m_pipeline; }
        VkPipelineLayout           getPipelineLayout() const { return m_pipeline_layout; }
        std::vector<VkFramebuffer> getSwapChainFramebuffers() const { return m_swap_chain_framebuffers; }

        // 静态工厂方法
        static std::shared_ptr<GPUPipeline> createBasicTrianglePipeline(VkDevice device, VkRenderPass render_pass);
        static std::shared_ptr<GPUPipeline> createDepthTestPipeline(VkDevice device, VkRenderPass render_pass);
        static std::shared_ptr<GPUPipeline> createWireframePipeline(VkDevice device, VkRenderPass render_pass);

    private:
        void createShaders();
        void createGraphicsPipeline();
        void createFramebuffers();
        
        // 使用构建器创建管道
        void createPipelineWithBuilder(const GPUPipelineBuilderConfig& config, VkRenderPass render_pass);

        // TODO : add pipeline resource
        std::vector<std::string> m_shader_paths;
        std::vector<ShaderType>  m_types;
        std::vector<std::string> m_entry_points;

        std::vector<std::shared_ptr<GPUShader>> m_shaders;

        VkDevice         m_device;
        VkPipeline       m_pipeline;
        VkPipelineLayout m_pipeline_layout;

        std::vector<VkFramebuffer> m_swap_chain_framebuffers;
    };
} // namespace Piccolo