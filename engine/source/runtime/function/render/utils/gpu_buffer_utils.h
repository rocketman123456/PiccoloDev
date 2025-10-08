#pragma once

#include <string>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    // 基础内存类型查找函数
    uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // 缓冲区上传相关结构
    struct BufferUploadInfo
    {
        VkBuffer       buffer;
        VkDeviceMemory memory;
        size_t         size;
        size_t         offset;
        void*          data;
        bool           use_staging;
    };

    // 暂存缓冲区管理器
    class StagingBufferManager
    {
    public:
        StagingBufferManager(VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue queue);
        ~StagingBufferManager();

        // 创建暂存缓冲区
        VkBuffer createStagingBuffer(size_t size, VkDeviceMemory& memory);

        // 销毁暂存缓冲区
        void destroyStagingBuffer(VkBuffer buffer, VkDeviceMemory memory);

        // 上传数据到GPU缓冲区
        void uploadToBuffer(VkBuffer dst_buffer, void* data, size_t size, size_t offset = 0);

        // 批量上传
        void uploadMultipleBuffers(const std::vector<BufferUploadInfo>& uploads);

        // 清理所有暂存缓冲区
        void cleanup();

    private:
        VkDevice         m_device;
        VkPhysicalDevice m_physical_device;
        VkCommandPool    m_command_pool;
        VkQueue          m_queue;

        struct StagingBuffer
        {
            VkBuffer       buffer;
            VkDeviceMemory memory;
            size_t         size;
            bool           in_use;
        };

        std::vector<StagingBuffer> m_staging_buffers;
        size_t                     m_current_offset;
    };

    // GPU缓冲区工具函数
    namespace GPUBufferUtility
    {
        // 基础缓冲区操作
        VkBuffer       createBuffer(VkDevice device, size_t size, VkBufferUsageFlags usage);
        VkDeviceMemory allocateBufferMemory(VkDevice device, VkPhysicalDevice physical_device, VkBuffer buffer, VkMemoryPropertyFlags properties);
        void           bindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset = 0);

        // 缓冲区上传函数
        void uploadDataToBuffer(
            VkDevice         device,
            VkPhysicalDevice physical_device,
            VkCommandPool    command_pool,
            VkQueue          queue,
            VkBuffer         dst_buffer,
            void*            data,
            size_t           size,
            size_t           offset = 0
        );

        // 使用暂存缓冲区上传
        void uploadDataWithStaging(
            VkDevice         device,
            VkPhysicalDevice physical_device,
            VkCommandPool    command_pool,
            VkQueue          queue,
            VkBuffer         dst_buffer,
            void*            data,
            size_t           size,
            size_t           offset = 0
        );

        // 直接映射上传（适用于主机可见内存）
        void uploadDataDirect(VkDevice device, VkDeviceMemory memory, void* data, size_t size, size_t offset = 0);

        // 缓冲区复制
        void copyBuffer(
            VkDevice      device,
            VkCommandPool command_pool,
            VkQueue       queue,
            VkBuffer      src_buffer,
            VkBuffer      dst_buffer,
            size_t        size,
            VkDeviceSize  src_offset = 0,
            VkDeviceSize  dst_offset = 0
        );

        // 缓冲区到图像复制
        void copyBufferToImage(
            VkDevice      device,
            VkCommandPool command_pool,
            VkQueue       queue,
            VkBuffer      src_buffer,
            VkImage       dst_image,
            VkExtent3D    extent,
            uint32_t      layer_count = 1,
            uint32_t      mip_level   = 0
        );

        // 内存管理工具
        size_t       alignSize(size_t size, size_t alignment);
        VkDeviceSize getBufferAlignment(VkPhysicalDevice physical_device, VkBufferUsageFlags usage_flags);
        VkDeviceSize getImageAlignment(VkPhysicalDevice physical_device, VkFormat format);

        // 内存映射工具
        void* mapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
        void  unmapMemory(VkDevice device, VkDeviceMemory memory);
        void  flushMappedMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
        void  invalidateMappedMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

        // 缓冲区验证和调试
        bool validateBuffer(VkDevice device, VkBuffer buffer);
        void printBufferInfo(VkDevice device, VkBuffer buffer, const std::string& name = "");
        void printMemoryInfo(VkPhysicalDevice physical_device, VkDeviceMemory memory, const std::string& name = "");

        // 高级缓冲区操作
        void updateBufferData(VkDevice device, VkDeviceMemory memory, void* data, size_t size, size_t offset = 0);
        void readBufferData(VkDevice device, VkDeviceMemory memory, void* data, size_t size, size_t offset = 0);

        // 缓冲区池管理
        class BufferPool
        {
        public:
            BufferPool(VkDevice device, VkPhysicalDevice physical_device, size_t buffer_size, VkBufferUsageFlags usage_flags);
            ~BufferPool();

            VkBuffer allocateBuffer();
            void     freeBuffer(VkBuffer buffer);
            void     cleanup();

        private:
            VkDevice           m_device;
            VkPhysicalDevice   m_physical_device;
            size_t             m_buffer_size;
            VkBufferUsageFlags m_usage_flags;

            struct PooledBuffer
            {
                VkBuffer       buffer;
                VkDeviceMemory memory;
                bool           in_use;
            };

            std::vector<PooledBuffer> m_buffers;
            std::vector<size_t>       m_free_indices;
        };

        // 批量操作工具
        void batchUploadBuffers(
            VkDevice                                                          device,
            VkPhysicalDevice                                                  physical_device,
            VkCommandPool                                                     command_pool,
            VkQueue                                                           queue,
            const std::vector<std::pair<VkBuffer, std::pair<void*, size_t>>>& uploads
        );

        void batchCopyBuffers(VkDevice device, VkCommandPool command_pool, VkQueue queue, const std::vector<std::tuple<VkBuffer, VkBuffer, size_t>>& copies);

        // 性能优化工具
        void   optimizeBufferLayout(VkPhysicalDevice physical_device, std::vector<VkBuffer>& buffers);
        size_t calculateOptimalStagingBufferSize(VkPhysicalDevice physical_device, size_t total_data_size);

        // 错误处理和验证
        bool        checkBufferSupport(VkPhysicalDevice physical_device, VkBufferUsageFlags usage_flags);
        std::string getBufferUsageString(VkBufferUsageFlags usage_flags);
        std::string getMemoryPropertyString(VkMemoryPropertyFlags property_flags);
    } // namespace GPUBufferUtility
} // namespace Piccolo