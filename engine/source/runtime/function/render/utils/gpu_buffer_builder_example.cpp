#include "runtime/function/render/utils/gpu_buffer_builder_example.h"
#include "runtime/function/render/gpu_render_resource_manager.h"

#include "runtime/core/base/macro.h"

namespace Piccolo
{
    void GPUBufferBuilderExample::demonstrateBufferCreation(VkDevice device, VkPhysicalDevice physical_device)
    {
        LOG_INFO("=== GPU缓冲区构建器使用示例 ===");

        // 1. 创建顶点缓冲区
        auto vertex_buffer = createVertexBufferExample(device, physical_device);
        LOG_INFO("创建顶点缓冲区成功，大小: {} 字节", vertex_buffer.size);

        // 2. 创建统一缓冲区
        auto uniform_buffer = createUniformBufferExample(device, physical_device);
        LOG_INFO("创建统一缓冲区成功，大小: {} 字节", uniform_buffer.size);

        // 3. 创建存储缓冲区
        auto storage_buffer = createStorageBufferExample(device, physical_device);
        LOG_INFO("创建存储缓冲区成功，大小: {} 字节", storage_buffer.size);

        // 4. 清理资源
        GPUBufferUtils::destroyBuffer(device, vertex_buffer);
        GPUBufferUtils::destroyBuffer(device, uniform_buffer);
        GPUBufferUtils::destroyBuffer(device, storage_buffer);

        LOG_INFO("=== 缓冲区创建示例完成 ===");
    }

    GPUBufferInfo GPUBufferBuilderExample::createVertexBufferExample(VkDevice device, VkPhysicalDevice physical_device)
    {
        // 使用构建器创建顶点缓冲区
        GPUBufferBuilder builder(device, physical_device);
        
        return builder.setName("ExampleVertexBuffer")
                     .setDescription("示例顶点缓冲区")
                     .asVertexBuffer(1024 * 1024) // 1MB
                     .setUsage(BufferUsage::Static)
                     .build();
    }

    GPUBufferInfo GPUBufferBuilderExample::createUniformBufferExample(VkDevice device, VkPhysicalDevice physical_device)
    {
        // 使用预定义配置创建统一缓冲区
        auto config = GPUBufferConfigFactory::createUniformBufferConfig(256, true); // 256字节，动态
        config.name = "ExampleUniformBuffer";
        config.description = "示例统一缓冲区";

        GPUBufferBuilder builder(device, physical_device);
        return builder.setName(config.name)
                     .setDescription(config.description)
                     .setSize(config.size)
                     .setType(config.type)
                     .setUsage(config.usage)
                     .setMemoryType(config.memory_type)
                     .setUsageFlags(config.usage_flags)
                     .setMemoryPropertyFlags(config.memory_property_flags)
                     .build();
    }

    GPUBufferInfo GPUBufferBuilderExample::createStorageBufferExample(VkDevice device, VkPhysicalDevice physical_device)
    {
        // 使用便捷方法创建存储缓冲区
        GPUBufferBuilder builder(device, physical_device);
        
        return builder.setName("ExampleStorageBuffer")
                     .setDescription("示例存储缓冲区")
                     .asStorageBuffer(512 * 1024) // 512KB
                     .setUsage(BufferUsage::Dynamic)
                     .setMemoryType(BufferMemoryType::HostVisible)
                     .build();
    }

    void GPUBufferBuilderExample::demonstrateResourceManagerUsage(VkDevice device, VkPhysicalDevice physical_device)
    {
        LOG_INFO("=== 使用资源管理器创建缓冲区示例 ===");

        // 创建资源管理器
        GPURenderResourceManager resource_manager(device, physical_device);

        // 1. 使用便捷方法创建缓冲区
        auto vertex_buffer = resource_manager.createVertexBuffer("ManagedVertexBuffer", 1024 * 1024, false);
        LOG_INFO("通过资源管理器创建顶点缓冲区: {} 字节", vertex_buffer.size);

        auto uniform_buffer = resource_manager.createUniformBuffer("ManagedUniformBuffer", 256, true);
        LOG_INFO("通过资源管理器创建统一缓冲区: {} 字节", uniform_buffer.size);

        // 2. 使用配置创建缓冲区
        auto config = GPUBufferConfigFactory::createStorageBufferConfig(512 * 1024, false);
        config.name = "ManagedStorageBuffer";
        config.description = "通过资源管理器管理的存储缓冲区";

        auto storage_buffer = resource_manager.createBuffer("ManagedStorageBuffer", config);
        LOG_INFO("通过资源管理器创建存储缓冲区: {} 字节", storage_buffer.size);

        // 3. 获取缓冲区
        auto retrieved_buffer = resource_manager.getBuffer("ManagedVertexBuffer");
        if (retrieved_buffer.buffer != VK_NULL_HANDLE)
        {
            LOG_INFO("成功获取缓冲区: {}", retrieved_buffer.size);
        }

        // 4. 获取统计信息
        LOG_INFO("当前管理的缓冲区数量: {}", resource_manager.getBufferCount());

        // 5. 清理所有资源（资源管理器会自动清理）
        resource_manager.clear();
        LOG_INFO("资源管理器已清理所有缓冲区");

        LOG_INFO("=== 资源管理器使用示例完成 ===");
    }

} // namespace Piccolo
