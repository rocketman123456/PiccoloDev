#pragma once

#include <string>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    /**
     * @brief 查找合适的内存类型
     * 
     * 根据内存类型过滤器和属性要求查找最适合的内存类型索引
     * 
     * @param physical_device 物理设备
     * @param type_filter 内存类型过滤器（位掩码）
     * @param properties 所需的内存属性
     * @return 内存类型索引，失败返回-1
     */
    uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);

    /**
     * @brief GPU缓冲区工具函数命名空间
     * 
     * 提供各种Vulkan缓冲区操作的实用工具函数，包括：
     * - 缓冲区的创建和销毁
     * - 内存分配和管理
     * - 数据上传和下载
     * - 缓冲区复制操作
     * - 性能优化工具
     */
    namespace GPUBufferUtility
    {
        // ========== 基础缓冲区操作 ==========
        
        /**
         * @brief 创建Vulkan缓冲区
         * 
         * @param device Vulkan设备
         * @param size 缓冲区大小（字节）
         * @param usage 缓冲区使用标志
         * @return 创建的缓冲区句柄，失败返回VK_NULL_HANDLE
         */
        VkBuffer createBuffer(VkDevice device, size_t size, VkBufferUsageFlags usage);
        
        /**
         * @brief 为缓冲区分配内存
         * 
         * @param device Vulkan设备
         * @param physical_device 物理设备
         * @param buffer 目标缓冲区
         * @param properties 内存属性要求
         * @return 分配的内存句柄，失败返回VK_NULL_HANDLE
         */
        VkDeviceMemory allocateBufferMemory(VkDevice device, VkPhysicalDevice physical_device, VkBuffer buffer, VkMemoryPropertyFlags properties);
        
        /**
         * @brief 绑定缓冲区内存
         * 
         * @param device Vulkan设备
         * @param buffer 缓冲区
         * @param memory 内存
         * @param offset 内存偏移量
         */
        void bindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset = 0);

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

        // 暂存缓冲区管理
        VkBuffer createStagingBuffer(VkDevice device, VkPhysicalDevice physical_device, size_t size, VkDeviceMemory& memory);
        void     destroyStagingBuffer(VkDevice device, VkBuffer buffer, VkDeviceMemory memory);

        // 使用暂存缓冲区上传数据
        void uploadDataWithStagingBuffer(
            VkDevice         device,
            VkPhysicalDevice physical_device,
            VkCommandPool    command_pool,
            VkQueue          queue,
            VkBuffer         dst_buffer,
            void*            data,
            size_t           size,
            size_t           offset = 0
        );

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