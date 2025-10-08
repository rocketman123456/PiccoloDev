#pragma once

// #include "runtime/function/render/gpu_resource.h"

#include <volk.h>

#include <memory>
#include <string>
#include <vector>

namespace Piccolo
{
    // 缓冲区类型枚举
    enum class BufferType
    {
        Vertex,
        Index,
        Uniform,
        Storage,
        Staging,
        Indirect,
        AccelerationStructure
    };

    // 缓冲区使用模式
    enum class BufferUsage
    {
        Static,  // 创建后不经常修改
        Dynamic, // 经常修改
        Stream   // 每帧都修改
    };

    // 缓冲区内存类型
    enum class BufferMemoryType
    {
        Device,     // 仅在GPU上
        Host,       // 在CPU上，可映射
        HostVisible // 在CPU上可见，但GPU也可访问
    };

    // 缓冲区配置结构
    struct GPUBufferConfig
    {
        std::string name;
        std::string description;

        BufferType            type                  = BufferType::Vertex;
        BufferUsage           usage                 = BufferUsage::Static;
        BufferMemoryType      memory_type           = BufferMemoryType::Device;
        VkBufferUsageFlags    usage_flags           = 0;
        VkMemoryPropertyFlags memory_property_flags = 0;

        size_t size         = 0;
        void*  initial_data = nullptr;

        // 高级选项
        bool persistent = false; // 持久化映射
        bool coherent   = false; // 内存一致性
        bool cached     = false; // CPU缓存
    };

    // 缓冲区信息结构
    struct GPUBufferInfo
    {
        VkBuffer       buffer        = VK_NULL_HANDLE;
        VkDeviceMemory memory        = VK_NULL_HANDLE;
        void*          mapped_memory = nullptr;
        size_t         size          = 0;
        size_t         alignment     = 0;
        bool           is_mapped     = false;
    };

    // GPU缓冲区构建器类
    class GPUBufferBuilder
    {
    public:
        explicit GPUBufferBuilder(VkDevice device, VkPhysicalDevice physical_device);
        ~GPUBufferBuilder() = default;

        // 基本配置
        GPUBufferBuilder& setName(const std::string& name);
        GPUBufferBuilder& setDescription(const std::string& description);
        GPUBufferBuilder& setSize(size_t size);
        GPUBufferBuilder& setInitialData(void* data, size_t data_size = 0);

        // 缓冲区类型配置
        GPUBufferBuilder& setType(BufferType type);
        GPUBufferBuilder& setUsage(BufferUsage usage);
        GPUBufferBuilder& setMemoryType(BufferMemoryType memory_type);

        // 直接设置Vulkan标志
        GPUBufferBuilder& setUsageFlags(VkBufferUsageFlags flags);
        GPUBufferBuilder& setMemoryPropertyFlags(VkMemoryPropertyFlags flags);

        // 高级选项
        GPUBufferBuilder& setPersistent(bool persistent = true);
        GPUBufferBuilder& setCoherent(bool coherent = true);
        GPUBufferBuilder& setCached(bool cached = true);

        // 便捷方法
        GPUBufferBuilder& asVertexBuffer(size_t size);
        GPUBufferBuilder& asIndexBuffer(size_t size);
        GPUBufferBuilder& asUniformBuffer(size_t size);
        GPUBufferBuilder& asStorageBuffer(size_t size);
        GPUBufferBuilder& asStagingBuffer(size_t size);
        GPUBufferBuilder& asIndirectBuffer(size_t size);

        // 构建缓冲区
        GPUBufferInfo build();
        GPUBufferInfo buildWithData(void* data, size_t data_size);

        // 重置构建器
        void reset();

        // 获取当前配置
        const GPUBufferConfig& getConfig() const { return m_config; }

    private:
        void     validateConfig() const;
        void     setupDefaultFlags();
        uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
        void     copyDataToBuffer(VkBuffer buffer, void* data, size_t data_size);

        VkDevice         m_device;
        VkPhysicalDevice m_physical_device;
        GPUBufferConfig  m_config;
    };

    // 预定义配置工厂
    class GPUBufferConfigFactory
    {
    public:
        // 顶点缓冲区配置
        static GPUBufferConfig createVertexBufferConfig(size_t size, bool dynamic = false);

        // 索引缓冲区配置
        static GPUBufferConfig createIndexBufferConfig(size_t size, bool dynamic = false);

        // 统一缓冲区配置
        static GPUBufferConfig createUniformBufferConfig(size_t size, bool dynamic = true);

        // 存储缓冲区配置
        static GPUBufferConfig createStorageBufferConfig(size_t size, bool dynamic = false);

        // 暂存缓冲区配置
        static GPUBufferConfig createStagingBufferConfig(size_t size);

        // 间接缓冲区配置
        static GPUBufferConfig createIndirectBufferConfig(size_t size, bool dynamic = false);

        // 加速结构缓冲区配置
        static GPUBufferConfig createAccelerationStructureBufferConfig(size_t size);
    };

    // 缓冲区工具类
    class GPUBufferUtils
    {
    public:
        // 创建临时暂存缓冲区
        static GPUBufferInfo createStagingBuffer(VkDevice device, VkPhysicalDevice physical_device, size_t size);

        // 复制数据到缓冲区
        static void copyDataToBuffer(VkDevice device, VkBuffer buffer, void* data, size_t data_size);

        // 复制缓冲区到缓冲区
        static void copyBuffer(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkBuffer src_buffer, VkBuffer dst_buffer, size_t size);

        // 映射缓冲区内存
        static void* mapBuffer(VkDevice device, VkDeviceMemory memory, size_t offset = 0, size_t size = VK_WHOLE_SIZE);

        // 取消映射缓冲区内存
        static void unmapBuffer(VkDevice device, VkDeviceMemory memory);

        // 刷新映射的内存
        static void flushMappedMemory(VkDevice device, VkDeviceMemory memory, size_t offset = 0, size_t size = VK_WHOLE_SIZE);

        // 销毁缓冲区
        static void destroyBuffer(VkDevice device, const GPUBufferInfo& buffer_info);

        // 获取缓冲区对齐要求
        static size_t getBufferAlignment(VkPhysicalDevice physical_device, VkBufferUsageFlags usage_flags);
    };

} // namespace Piccolo
