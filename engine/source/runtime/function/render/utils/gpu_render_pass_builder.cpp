#include "runtime/function/render/utils/gpu_render_pass_builder.h"
#include "runtime/core/base/macro.h"

#include <stdexcept>

namespace Piccolo
{
    GPURenderPassBuilder::GPURenderPassBuilder(VkDevice device)
        : m_device(device)
    {
        reset();
    }

    GPURenderPassBuilder& GPURenderPassBuilder::setName(const std::string& name)
    {
        m_config.name = name;
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::setDescription(const std::string& description)
    {
        m_config.description = description;
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addColorAttachment(const AttachmentConfig& config)
    {
        AttachmentConfig color_config = config;
        if (color_config.final_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            color_config.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        m_config.attachments.push_back(color_config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addDepthAttachment(const AttachmentConfig& config)
    {
        AttachmentConfig depth_config = config;
        if (depth_config.final_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            depth_config.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        m_config.attachments.push_back(depth_config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addStencilAttachment(const AttachmentConfig& config)
    {
        AttachmentConfig stencil_config = config;
        if (stencil_config.final_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            stencil_config.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        m_config.attachments.push_back(stencil_config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addDepthStencilAttachment(const AttachmentConfig& config)
    {
        AttachmentConfig depth_stencil_config = config;
        if (depth_stencil_config.final_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            depth_stencil_config.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        m_config.attachments.push_back(depth_stencil_config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addResolveAttachment(const AttachmentConfig& config)
    {
        AttachmentConfig resolve_config = config;
        if (resolve_config.final_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            resolve_config.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        m_config.attachments.push_back(resolve_config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addSubpass(const SubpassConfig& config)
    {
        m_config.subpasses.push_back(config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addSubpass(const std::string& name)
    {
        SubpassConfig config;
        config.name = name;
        m_config.subpasses.push_back(config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addDependency(const SubpassDependencyConfig& config)
    {
        m_config.dependencies.push_back(config);
        return *this;
    }

    GPURenderPassBuilder& GPURenderPassBuilder::addDependency(uint32_t src_subpass, uint32_t dst_subpass)
    {
        SubpassDependencyConfig config;
        config.src_subpass     = src_subpass;
        config.dst_subpass     = dst_subpass;
        config.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        config.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        config.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        m_config.dependencies.push_back(config);
        return *this;
    }

    VkRenderPass GPURenderPassBuilder::build()
    {
        if (m_config.attachments.empty())
        {
            LOG_ERROR("No attachments configured for render pass: {}", m_config.name);
            return VK_NULL_HANDLE;
        }

        if (m_config.subpasses.empty())
        {
            LOG_ERROR("No subpasses configured for render pass: {}", m_config.name);
            return VK_NULL_HANDLE;
        }

        // 转换附件描述
        std::vector<VkAttachmentDescription> attachments;
        for (const auto& config : m_config.attachments)
        {
            VkAttachmentDescription attachment {};
            attachment.format         = config.format;
            attachment.samples        = config.samples;
            attachment.loadOp         = config.load_op;
            attachment.storeOp        = config.store_op;
            attachment.stencilLoadOp  = config.stencil_load_op;
            attachment.stencilStoreOp = config.stencil_store_op;
            attachment.initialLayout  = config.initial_layout;
            attachment.finalLayout    = config.final_layout;
            attachments.push_back(attachment);
        }

        // 转换子通道描述
        std::vector<VkSubpassDescription>               subpasses;
        std::vector<std::vector<VkAttachmentReference>> color_attachments_storage;
        std::vector<std::vector<VkAttachmentReference>> input_attachments_storage;
        std::vector<std::vector<VkAttachmentReference>> resolve_attachments_storage;
        std::vector<VkAttachmentReference>              depth_stencil_attachments_storage;

        for (size_t i = 0; i < m_config.subpasses.size(); ++i)
        {
            const auto& subpass_config = m_config.subpasses[i];

            VkSubpassDescription subpass {};
            subpass.pipelineBindPoint = subpass_config.bind_point;

            // 颜色附件
            if (!subpass_config.color_attachments.empty())
            {
                color_attachments_storage.push_back(subpass_config.color_attachments);
                subpass.colorAttachmentCount = static_cast<uint32_t>(subpass_config.color_attachments.size());
                subpass.pColorAttachments    = color_attachments_storage.back().data();
            }

            // 输入附件
            if (!subpass_config.input_attachments.empty())
            {
                input_attachments_storage.push_back(subpass_config.input_attachments);
                subpass.inputAttachmentCount = static_cast<uint32_t>(subpass_config.input_attachments.size());
                subpass.pInputAttachments    = input_attachments_storage.back().data();
            }

            // 解析附件
            if (!subpass_config.resolve_attachments.empty())
            {
                resolve_attachments_storage.push_back(subpass_config.resolve_attachments);
                subpass.pResolveAttachments = resolve_attachments_storage.back().data();
            }

            // 深度模板附件
            if (subpass_config.depth_stencil_attachment)
            {
                depth_stencil_attachments_storage.push_back(*subpass_config.depth_stencil_attachment);
                subpass.pDepthStencilAttachment = &depth_stencil_attachments_storage.back();
            }

            // 保留附件
            if (!subpass_config.preserve_attachments.empty())
            {
                subpass.preserveAttachmentCount = static_cast<uint32_t>(subpass_config.preserve_attachments.size());
                subpass.pPreserveAttachments    = subpass_config.preserve_attachments.data();
            }

            subpasses.push_back(subpass);
        }

        // 转换子通道依赖
        std::vector<VkSubpassDependency> dependencies;
        for (const auto& config : m_config.dependencies)
        {
            VkSubpassDependency dependency {};
            dependency.srcSubpass      = config.src_subpass;
            dependency.dstSubpass      = config.dst_subpass;
            dependency.srcStageMask    = config.src_stage_mask;
            dependency.srcAccessMask   = config.src_access_mask;
            dependency.dstStageMask    = config.dst_stage_mask;
            dependency.dstAccessMask   = config.dst_access_mask;
            dependency.dependencyFlags = config.dependency_flags;
            dependencies.push_back(dependency);
        }

        // 创建渲染通道
        VkRenderPassCreateInfo render_pass_info {};
        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        render_pass_info.pAttachments    = attachments.data();
        render_pass_info.subpassCount    = static_cast<uint32_t>(subpasses.size());
        render_pass_info.pSubpasses      = subpasses.data();
        render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
        render_pass_info.pDependencies   = dependencies.data();

        VkRenderPass render_pass;
        VkResult     result = vkCreateRenderPass(m_device, &render_pass_info, nullptr, &render_pass);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create render pass: {}", m_config.name);
            return VK_NULL_HANDLE;
        }

        LOG_DEBUG("Successfully created render pass: {}", m_config.name);
        return render_pass;
    }

    void GPURenderPassBuilder::reset()
    {
        m_config             = {};
        m_config.name        = "Unnamed Render Pass";
        m_config.description = "No description";
    }

    // 预定义渲染通道配置工厂实现
    GPURenderPassConfig GPURenderPassConfigFactory::createBasicColorPass(VkFormat color_format)
    {
        GPURenderPassConfig config;
        config.name        = "Basic Color Pass";
        config.description = "Basic render pass with color attachment only";

        // 颜色附件
        AttachmentConfig color_attachment;
        color_attachment.format         = color_format;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.final_layout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        config.attachments.push_back(color_attachment);

        // 子通道
        SubpassConfig subpass;
        subpass.name = "Color Subpass";
        subpass.color_attachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        config.subpasses.push_back(subpass);

        // 子通道依赖
        SubpassDependencyConfig dependency;
        dependency.src_subpass     = VK_SUBPASS_EXTERNAL;
        dependency.dst_subpass     = 0;
        dependency.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        config.dependencies.push_back(dependency);

        return config;
    }

    GPURenderPassConfig GPURenderPassConfigFactory::createDepthColorPass(VkFormat color_format, VkFormat depth_format)
    {
        GPURenderPassConfig config;
        config.name        = "Depth Color Pass";
        config.description = "Render pass with color and depth attachments";

        // 颜色附件
        AttachmentConfig color_attachment;
        color_attachment.format         = color_format;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.final_layout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        config.attachments.push_back(color_attachment);

        // 深度附件
        AttachmentConfig depth_attachment;
        depth_attachment.format         = depth_format;
        depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.store_op       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.final_layout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        config.attachments.push_back(depth_attachment);

        // 子通道
        SubpassConfig subpass;
        subpass.name = "Depth Color Subpass";
        subpass.color_attachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        subpass.depth_stencil_attachment = new VkAttachmentReference({1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
        config.subpasses.push_back(subpass);

        // 子通道依赖
        SubpassDependencyConfig dependency;
        dependency.src_subpass     = VK_SUBPASS_EXTERNAL;
        dependency.dst_subpass     = 0;
        dependency.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        config.dependencies.push_back(dependency);

        return config;
    }

    GPURenderPassConfig GPURenderPassConfigFactory::createMultisamplePass(VkFormat color_format, VkFormat depth_format, VkSampleCountFlagBits samples)
    {
        GPURenderPassConfig config;
        config.name        = "Multisample Pass";
        config.description = "Render pass with multisample color and depth attachments";

        // 多重采样颜色附件
        AttachmentConfig color_attachment;
        color_attachment.format         = color_format;
        color_attachment.samples        = samples;
        color_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.store_op       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.final_layout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        config.attachments.push_back(color_attachment);

        // 解析颜色附件
        AttachmentConfig resolve_attachment;
        resolve_attachment.format         = color_format;
        resolve_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        resolve_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolve_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        resolve_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolve_attachment.final_layout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        config.attachments.push_back(resolve_attachment);

        // 多重采样深度附件
        AttachmentConfig depth_attachment;
        depth_attachment.format         = depth_format;
        depth_attachment.samples        = samples;
        depth_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.store_op       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.final_layout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        config.attachments.push_back(depth_attachment);

        // 子通道
        SubpassConfig subpass;
        subpass.name = "Multisample Subpass";
        subpass.color_attachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        subpass.resolve_attachments.push_back({1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        subpass.depth_stencil_attachment = new VkAttachmentReference({2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
        config.subpasses.push_back(subpass);

        // 子通道依赖
        SubpassDependencyConfig dependency;
        dependency.src_subpass     = VK_SUBPASS_EXTERNAL;
        dependency.dst_subpass     = 0;
        dependency.src_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        config.dependencies.push_back(dependency);

        return config;
    }

    GPURenderPassConfig GPURenderPassConfigFactory::createPostProcessPass(VkFormat color_format)
    {
        GPURenderPassConfig config;
        config.name        = "Post Process Pass";
        config.description = "Render pass for post-processing effects";

        // 颜色附件
        AttachmentConfig color_attachment;
        color_attachment.format         = color_format;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.initial_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        color_attachment.final_layout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        config.attachments.push_back(color_attachment);

        // 子通道
        SubpassConfig subpass;
        subpass.name = "Post Process Subpass";
        subpass.color_attachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        config.subpasses.push_back(subpass);

        // 子通道依赖
        SubpassDependencyConfig dependency;
        dependency.src_subpass     = VK_SUBPASS_EXTERNAL;
        dependency.dst_subpass     = 0;
        dependency.src_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.src_access_mask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        config.dependencies.push_back(dependency);

        return config;
    }

    GPURenderPassConfig GPURenderPassConfigFactory::createShadowMapPass(VkFormat depth_format)
    {
        GPURenderPassConfig config;
        config.name        = "Shadow Map Pass";
        config.description = "Render pass for shadow map generation";

        // 深度附件
        AttachmentConfig depth_attachment;
        depth_attachment.format         = depth_format;
        depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.final_layout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        config.attachments.push_back(depth_attachment);

        // 子通道
        SubpassConfig subpass;
        subpass.name                     = "Shadow Map Subpass";
        subpass.depth_stencil_attachment = new VkAttachmentReference({0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
        config.subpasses.push_back(subpass);

        // 子通道依赖
        SubpassDependencyConfig dependency;
        dependency.src_subpass     = VK_SUBPASS_EXTERNAL;
        dependency.dst_subpass     = 0;
        dependency.src_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.src_access_mask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dst_stage_mask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        config.dependencies.push_back(dependency);

        return config;
    }

    GPURenderPassConfig GPURenderPassConfigFactory::createDeferredPass(VkFormat color_format, VkFormat normal_format, VkFormat depth_format)
    {
        GPURenderPassConfig config;
        config.name        = "Deferred Pass";
        config.description = "Render pass for deferred rendering";

        // G-Buffer 颜色附件
        AttachmentConfig albedo_attachment;
        albedo_attachment.format         = color_format;
        albedo_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        albedo_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        albedo_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        albedo_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        albedo_attachment.final_layout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        config.attachments.push_back(albedo_attachment);

        // G-Buffer 法线附件
        AttachmentConfig normal_attachment;
        normal_attachment.format         = normal_format;
        normal_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        normal_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        normal_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        normal_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        normal_attachment.final_layout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        config.attachments.push_back(normal_attachment);

        // 深度附件
        AttachmentConfig depth_attachment;
        depth_attachment.format         = depth_format;
        depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.load_op        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.store_op       = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.final_layout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        config.attachments.push_back(depth_attachment);

        // 子通道
        SubpassConfig subpass;
        subpass.name = "G-Buffer Subpass";
        subpass.color_attachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        subpass.color_attachments.push_back({1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        subpass.depth_stencil_attachment = new VkAttachmentReference({2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
        config.subpasses.push_back(subpass);

        // 子通道依赖
        SubpassDependencyConfig dependency;
        dependency.src_subpass     = VK_SUBPASS_EXTERNAL;
        dependency.dst_subpass     = 0;
        dependency.src_stage_mask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.src_access_mask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dst_stage_mask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        config.dependencies.push_back(dependency);

        return config;
    }
} // namespace Piccolo
