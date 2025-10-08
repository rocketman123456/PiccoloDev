#pragma once

#include "runtime/function/render/utils/gpu_buffer_builder.h"

namespace Piccolo
{
    // GPU缓冲区构建器使用示例
    class GPUBufferBuilderExample
    {
    public:
        // 演示如何创建不同类型的缓冲区
        static void demonstrateBufferCreation(VkDevice device, VkPhysicalDevice physical_device);

        // 创建顶点缓冲区示例
        static GPUBufferInfo createVertexBufferExample(VkDevice device, VkPhysicalDevice physical_device);

        // 创建统一缓冲区示例
        static GPUBufferInfo createUniformBufferExample(VkDevice device, VkPhysicalDevice physical_device);

        // 创建存储缓冲区示例
        static GPUBufferInfo createStorageBufferExample(VkDevice device, VkPhysicalDevice physical_device);

        // 使用资源管理器创建缓冲区示例
        static void demonstrateResourceManagerUsage(VkDevice device, VkPhysicalDevice physical_device);
    };

} // namespace Piccolo
