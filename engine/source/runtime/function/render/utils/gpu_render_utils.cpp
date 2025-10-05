#include "runtime/function/render/utils/gpu_render_utils.h"
#include "runtime/core/base/macro.h"

#include <algorithm>
#include <stdexcept>

namespace Piccolo
{
    // GPURenderUtils 命名空间实现
    namespace GPURenderUtils
    {
        VkViewport createViewport(float x, float y, float width, float height, float min_depth, float max_depth)
        {
            VkViewport viewport {};
            viewport.x        = x;
            viewport.y        = y;
            viewport.width    = width;
            viewport.height   = height;
            viewport.minDepth = min_depth;
            viewport.maxDepth = max_depth;
            return viewport;
        }

        VkRect2D createScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            VkRect2D scissor {};
            scissor.offset = {x, y};
            scissor.extent = {width, height};
            return scissor;
        }

        VkClearValue createClearColor(float r, float g, float b, float a)
        {
            VkClearValue clear_value {};
            clear_value.color = {
                {r, g, b, a}
            };
            return clear_value;
        }

        VkClearValue createClearDepthStencil(float depth, uint32_t stencil)
        {
            VkClearValue clear_value {};
            clear_value.depthStencil = {depth, stencil};
            return clear_value;
        }

        VkRect2D createRenderArea(int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            VkRect2D render_area {};
            render_area.offset = {x, y};
            render_area.extent = {width, height};
            return render_area;
        }

        bool isFormatSupported(VkPhysicalDevice physical_device, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return true;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return true;
            }

            return false;
        }

        VkFormat findBestDepthFormat(VkPhysicalDevice physical_device)
        {
            std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

            for (VkFormat format : candidates)
            {
                if (isFormatSupported(physical_device, format, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
                {
                    return format;
                }
            }

            return VK_FORMAT_UNDEFINED;
        }

        VkFormat findBestDepthStencilFormat(VkPhysicalDevice physical_device)
        {
            std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

            for (VkFormat format : candidates)
            {
                if (isFormatSupported(physical_device, format, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
                {
                    return format;
                }
            }

            return VK_FORMAT_UNDEFINED;
        }

        VkImageView createImageView(
            VkDevice           device,
            VkImage            image,
            VkFormat           format,
            VkImageAspectFlags aspect_flags,
            uint32_t           base_mip_level,
            uint32_t           level_count,
            uint32_t           base_array_layer,
            uint32_t           layer_count
        )
        {
            VkImageViewCreateInfo view_info {};
            view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image                           = image;
            view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format                          = format;
            view_info.subresourceRange.aspectMask     = aspect_flags;
            view_info.subresourceRange.baseMipLevel   = base_mip_level;
            view_info.subresourceRange.levelCount     = level_count;
            view_info.subresourceRange.baseArrayLayer = base_array_layer;
            view_info.subresourceRange.layerCount     = layer_count;

            VkImageView image_view;
            if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to create texture image view!");
                throw std::runtime_error("Failed to create texture image view!");
            }

            return image_view;
        }

        VkSampler createSampler(
            VkDevice             device,
            VkFilter             mag_filter,
            VkFilter             min_filter,
            VkSamplerAddressMode address_mode_u,
            VkSamplerAddressMode address_mode_v,
            VkSamplerAddressMode address_mode_w,
            float                max_anisotropy
        )
        {
            VkSamplerCreateInfo sampler_info {};
            sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_info.magFilter               = mag_filter;
            sampler_info.minFilter               = min_filter;
            sampler_info.addressModeU            = address_mode_u;
            sampler_info.addressModeV            = address_mode_v;
            sampler_info.addressModeW            = address_mode_w;
            sampler_info.anisotropyEnable        = VK_FALSE;
            sampler_info.maxAnisotropy           = max_anisotropy;
            sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            sampler_info.unnormalizedCoordinates = VK_FALSE;
            sampler_info.compareEnable           = VK_FALSE;
            sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
            sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

            VkSampler sampler;
            if (vkCreateSampler(device, &sampler_info, nullptr, &sampler) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to create texture sampler!");
                throw std::runtime_error("Failed to create texture sampler!");
            }

            return sampler;
        }

        VkDescriptorSetLayoutBinding
        createDescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptor_type, uint32_t descriptor_count, VkShaderStageFlags stage_flags)
        {
            VkDescriptorSetLayoutBinding layout_binding {};
            layout_binding.binding            = binding;
            layout_binding.descriptorType     = descriptor_type;
            layout_binding.descriptorCount    = descriptor_count;
            layout_binding.stageFlags         = stage_flags;
            layout_binding.pImmutableSamplers = nullptr;
            return layout_binding;
        }

        VkPushConstantRange createPushConstantRange(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size)
        {
            VkPushConstantRange push_constant_range {};
            push_constant_range.stageFlags = stage_flags;
            push_constant_range.offset     = offset;
            push_constant_range.size       = size;
            return push_constant_range;
        }

        VkImageMemoryBarrier createImageMemoryBarrier(
            VkImage            image,
            VkImageLayout      old_layout,
            VkImageLayout      new_layout,
            VkImageAspectFlags aspect_mask,
            uint32_t           base_mip_level,
            uint32_t           level_count,
            uint32_t           base_array_layer,
            uint32_t           layer_count
        )
        {
            VkImageMemoryBarrier barrier {};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = old_layout;
            barrier.newLayout                       = new_layout;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = image;
            barrier.subresourceRange.aspectMask     = aspect_mask;
            barrier.subresourceRange.baseMipLevel   = base_mip_level;
            barrier.subresourceRange.levelCount     = level_count;
            barrier.subresourceRange.baseArrayLayer = base_array_layer;
            barrier.subresourceRange.layerCount     = layer_count;
            return barrier;
        }

        VkBufferMemoryBarrier
        createBufferMemoryBarrier(VkBuffer buffer, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask, VkDeviceSize offset, VkDeviceSize size)
        {
            VkBufferMemoryBarrier barrier {};
            barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask       = src_access_mask;
            barrier.dstAccessMask       = dst_access_mask;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer              = buffer;
            barrier.offset              = offset;
            barrier.size                = size;
            return barrier;
        }
    } // namespace GPURenderUtils
} // namespace Piccolo