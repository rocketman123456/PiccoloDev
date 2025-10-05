#pragma once

#include <memory>
#include <string>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    // 渲染工具函数
    namespace GPURenderUtils
    {
        // 创建默认的视口
        VkViewport createViewport(float x, float y, float width, float height, float min_depth = 0.0f, float max_depth = 1.0f);

        // 创建默认的裁剪矩形
        VkRect2D createScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

        // 创建清除值
        VkClearValue createClearColor(float r, float g, float b, float a = 1.0f);
        VkClearValue createClearDepthStencil(float depth = 1.0f, uint32_t stencil = 0);

        // 创建渲染区域
        VkRect2D createRenderArea(int32_t x, int32_t y, uint32_t width, uint32_t height);

        // 检查格式支持
        bool isFormatSupported(VkPhysicalDevice physical_device, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features);

        // 查找最佳深度格式
        VkFormat findBestDepthFormat(VkPhysicalDevice physical_device);

        // 查找最佳深度模板格式
        VkFormat findBestDepthStencilFormat(VkPhysicalDevice physical_device);

        // 创建图像视图
        VkImageView createImageView(
            VkDevice           device,
            VkImage            image,
            VkFormat           format,
            VkImageAspectFlags aspect_flags,
            uint32_t           base_mip_level   = 0,
            uint32_t           level_count      = 1,
            uint32_t           base_array_layer = 0,
            uint32_t           layer_count      = 1
        );

        // 创建采样器
        VkSampler createSampler(
            VkDevice             device,
            VkFilter             mag_filter     = VK_FILTER_LINEAR,
            VkFilter             min_filter     = VK_FILTER_LINEAR,
            VkSamplerAddressMode address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VkSamplerAddressMode address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VkSamplerAddressMode address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            float                max_anisotropy = 1.0f
        );

        // 创建描述符集布局绑定
        VkDescriptorSetLayoutBinding createDescriptorSetLayoutBinding(
            uint32_t           binding,
            VkDescriptorType   descriptor_type,
            uint32_t           descriptor_count = 1,
            VkShaderStageFlags stage_flags      = VK_SHADER_STAGE_ALL
        );

        // 创建推送常量范围
        VkPushConstantRange createPushConstantRange(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size);

        // 内存屏障辅助函数
        VkImageMemoryBarrier createImageMemoryBarrier(
            VkImage            image,
            VkImageLayout      old_layout,
            VkImageLayout      new_layout,
            VkImageAspectFlags aspect_mask      = VK_IMAGE_ASPECT_COLOR_BIT,
            uint32_t           base_mip_level   = 0,
            uint32_t           level_count      = 1,
            uint32_t           base_array_layer = 0,
            uint32_t           layer_count      = 1
        );

        VkBufferMemoryBarrier createBufferMemoryBarrier(
            VkBuffer      buffer,
            VkAccessFlags src_access_mask,
            VkAccessFlags dst_access_mask,
            VkDeviceSize  offset = 0,
            VkDeviceSize  size   = VK_WHOLE_SIZE
        );
    } // namespace GPURenderUtils
} // namespace Piccolo