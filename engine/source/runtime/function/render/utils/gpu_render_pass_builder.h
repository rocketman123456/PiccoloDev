#pragma once

#include "runtime/function/render/utils/gpu_utils.h"

#include <string>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    // 附件配置
    struct AttachmentConfig
    {
        VkFormat              format           = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits samples          = VK_SAMPLE_COUNT_1_BIT;
        VkAttachmentLoadOp    load_op          = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp   store_op         = VK_ATTACHMENT_STORE_OP_STORE;
        VkAttachmentLoadOp    stencil_load_op  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        VkAttachmentStoreOp   stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkImageLayout         initial_layout   = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout         final_layout     = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    // 子通道配置
    struct SubpassConfig
    {
        std::string         name;
        VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // 颜色附件引用
        std::vector<VkAttachmentReference> color_attachments;
        std::vector<VkAttachmentReference> input_attachments;
        std::vector<VkAttachmentReference> resolve_attachments;

        // 深度模板附件引用
        VkAttachmentReference* depth_stencil_attachment = nullptr;

        // 保留附件
        std::vector<uint32_t> preserve_attachments;
    };

    // 子通道依赖配置
    struct SubpassDependencyConfig
    {
        uint32_t             src_subpass      = VK_SUBPASS_EXTERNAL;
        uint32_t             dst_subpass      = 0;
        VkPipelineStageFlags src_stage_mask   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkAccessFlags        src_access_mask  = 0;
        VkPipelineStageFlags dst_stage_mask   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkAccessFlags        dst_access_mask  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkDependencyFlags    dependency_flags = 0;
    };

    // 渲染通道配置
    struct GPURenderPassConfig
    {
        std::string name;
        std::string description;

        std::vector<AttachmentConfig>        attachments;
        std::vector<SubpassConfig>           subpasses;
        std::vector<SubpassDependencyConfig> dependencies;
    };

    // 渲染通道构建器
    class GPURenderPassBuilder
    {
    public:
        explicit GPURenderPassBuilder(VkDevice device);
        ~GPURenderPassBuilder() = default;

        // 基本配置
        GPURenderPassBuilder& setName(const std::string& name);
        GPURenderPassBuilder& setDescription(const std::string& description);

        // 附件配置
        GPURenderPassBuilder& addColorAttachment(const AttachmentConfig& config);
        GPURenderPassBuilder& addDepthAttachment(const AttachmentConfig& config);
        GPURenderPassBuilder& addStencilAttachment(const AttachmentConfig& config);
        GPURenderPassBuilder& addDepthStencilAttachment(const AttachmentConfig& config);
        GPURenderPassBuilder& addResolveAttachment(const AttachmentConfig& config);

        // 子通道配置
        GPURenderPassBuilder& addSubpass(const SubpassConfig& config);
        GPURenderPassBuilder& addSubpass(const std::string& name);

        // 子通道依赖配置
        GPURenderPassBuilder& addDependency(const SubpassDependencyConfig& config);
        GPURenderPassBuilder& addDependency(uint32_t src_subpass, uint32_t dst_subpass);

        // 构建渲染通道
        VkRenderPass build();

        // 重置构建器
        void reset();

    private:
        VkDevice            m_device;
        GPURenderPassConfig m_config;
    };

    // 预定义渲染通道配置工厂
    class GPURenderPassConfigFactory
    {
    public:
        // 基础颜色渲染通道
        static GPURenderPassConfig createBasicColorPass(VkFormat color_format);

        // 带深度测试的渲染通道
        static GPURenderPassConfig createDepthColorPass(VkFormat color_format, VkFormat depth_format);

        // 多重采样渲染通道
        static GPURenderPassConfig createMultisamplePass(VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits samples);

        // 后处理渲染通道
        static GPURenderPassConfig createPostProcessPass(VkFormat color_format);

        // 阴影贴图渲染通道
        static GPURenderPassConfig createShadowMapPass(VkFormat depth_format);

        // 延迟渲染通道
        static GPURenderPassConfig createDeferredPass(VkFormat color_format, VkFormat normal_format, VkFormat depth_format);
    };
} // namespace Piccolo
