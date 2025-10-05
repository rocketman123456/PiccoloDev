#pragma once

#include "runtime/function/render/utils/gpu_utils.h"
#include "runtime/function/render/utils/gpu_render_pass_builder.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <vector>
#include <memory>

namespace Piccolo
{
    class GPURenderPass
    {
    public:
        // 使用新的构建器创建渲染通道
        GPURenderPass(VkDevice device, const GPURenderPassConfig& config);
        
        // 保持向后兼容的构造函数
        GPURenderPass(VkDevice device);
        
        ~GPURenderPass();

        VkRenderPass getRenderPass() const { return m_render_pass; }

        // 静态工厂方法
        static std::shared_ptr<GPURenderPass> createBasicColorPass(VkDevice device, VkFormat color_format);
        static std::shared_ptr<GPURenderPass> createDepthColorPass(VkDevice device, VkFormat color_format, VkFormat depth_format);
        static std::shared_ptr<GPURenderPass> createMultisamplePass(VkDevice device, VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits samples);

    private:
        void createRenderPass();
        void createRenderPassWithBuilder(const GPURenderPassConfig& config);

        VkDevice     m_device;
        VkRenderPass m_render_pass;
    };
} // namespace Piccolo
