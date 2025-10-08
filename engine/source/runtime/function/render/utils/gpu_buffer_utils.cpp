#include "runtime/function/render/utils/gpu_buffer_utils.h"

#include "runtime/core/base/macro.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace Piccolo
{
    // 基础内存类型查找函数
    uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        LOG_ERROR("failed to find suitable memory type!");
        return -1;
    }

    // StagingBufferManager 实现
    StagingBufferManager::StagingBufferManager(VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue queue)
        : m_device(device)
        , m_physical_device(physical_device)
        , m_command_pool(command_pool)
        , m_queue(queue)
        , m_current_offset(0)
    {}

    StagingBufferManager::~StagingBufferManager() { cleanup(); }

    VkBuffer StagingBufferManager::createStagingBuffer(size_t size, VkDeviceMemory& memory)
    {
        VkBufferCreateInfo buffer_info {};
        buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size        = size;
        buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer;
        if (vkCreateBuffer(m_device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create staging buffer");
            return VK_NULL_HANDLE;
        }

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(m_device, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info {};
        alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex =
            find_memory_type(m_physical_device, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(m_device, &alloc_info, nullptr, &memory) != VK_SUCCESS)
        {
            vkDestroyBuffer(m_device, buffer, nullptr);
            LOG_ERROR("Failed to allocate staging buffer memory");
            return VK_NULL_HANDLE;
        }

        vkBindBufferMemory(m_device, buffer, memory, 0);

        m_staging_buffers.push_back({buffer, memory, size, false});
        return buffer;
    }

    void StagingBufferManager::destroyStagingBuffer(VkBuffer buffer, VkDeviceMemory memory)
    {
        vkDestroyBuffer(m_device, buffer, nullptr);
        vkFreeMemory(m_device, memory, nullptr);

        auto it = std::find_if(m_staging_buffers.begin(), m_staging_buffers.end(), [buffer](const StagingBuffer& sb) { return sb.buffer == buffer; });
        if (it != m_staging_buffers.end())
        {
            m_staging_buffers.erase(it);
        }
    }

    void StagingBufferManager::uploadToBuffer(VkBuffer dst_buffer, void* data, size_t size, size_t offset)
    {
        VkDeviceMemory staging_memory;
        VkBuffer       staging_buffer = createStagingBuffer(size, staging_memory);

        // 映射并复制数据
        void* mapped_data;
        vkMapMemory(m_device, staging_memory, 0, size, 0, &mapped_data);
        memcpy(mapped_data, data, size);
        vkUnmapMemory(m_device, staging_memory);

        // 复制缓冲区
        GPUBufferUtility::copyBuffer(m_device, m_command_pool, m_queue, staging_buffer, dst_buffer, size, 0, offset);

        // 清理暂存缓冲区
        destroyStagingBuffer(staging_buffer, staging_memory);
    }

    void StagingBufferManager::uploadMultipleBuffers(const std::vector<BufferUploadInfo>& uploads)
    {
        for (const auto& upload : uploads)
        {
            if (upload.use_staging)
            {
                uploadToBuffer(upload.buffer, upload.data, upload.size, upload.offset);
            }
            else
            {
                GPUBufferUtility::uploadDataDirect(m_device, upload.memory, upload.data, upload.size, upload.offset);
            }
        }
    }

    void StagingBufferManager::cleanup()
    {
        for (const auto& staging_buffer : m_staging_buffers)
        {
            vkDestroyBuffer(m_device, staging_buffer.buffer, nullptr);
            vkFreeMemory(m_device, staging_buffer.memory, nullptr);
        }
        m_staging_buffers.clear();
        m_current_offset = 0;
    }

    // GPUBufferUtility 命名空间实现
    namespace GPUBufferUtility
    {
        // 基础缓冲区操作
        VkBuffer createBuffer(VkDevice device, size_t size, VkBufferUsageFlags usage)
        {
            VkBufferCreateInfo buffer_info {};
            buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size        = size;
            buffer_info.usage       = usage;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkBuffer buffer;
            if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to create buffer");
                return VK_NULL_HANDLE;
            }

            return buffer;
        }

        VkDeviceMemory allocateBufferMemory(VkDevice device, VkPhysicalDevice physical_device, VkBuffer buffer, VkMemoryPropertyFlags properties)
        {
            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

            VkMemoryAllocateInfo alloc_info {};
            alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize  = mem_requirements.size;
            alloc_info.memoryTypeIndex = find_memory_type(physical_device, mem_requirements.memoryTypeBits, properties);

            VkDeviceMemory memory;
            if (vkAllocateMemory(device, &alloc_info, nullptr, &memory) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to allocate buffer memory");
                return VK_NULL_HANDLE;
            }

            return memory;
        }

        void bindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset)
        {
            vkBindBufferMemory(device, buffer, memory, offset);
        }

        // 缓冲区上传函数
        void uploadDataToBuffer(
            VkDevice         device,
            VkPhysicalDevice physical_device,
            VkCommandPool    command_pool,
            VkQueue          queue,
            VkBuffer         dst_buffer,
            void*            data,
            size_t           size,
            size_t           offset
        )
        {
            uploadDataWithStaging(device, physical_device, command_pool, queue, dst_buffer, data, size, offset);
        }

        void uploadDataWithStaging(
            VkDevice         device,
            VkPhysicalDevice physical_device,
            VkCommandPool    command_pool,
            VkQueue          queue,
            VkBuffer         dst_buffer,
            void*            data,
            size_t           size,
            size_t           offset
        )
        {
            // 创建暂存缓冲区
            VkBufferCreateInfo buffer_info {};
            buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.size        = size;
            buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkBuffer staging_buffer;
            if (vkCreateBuffer(device, &buffer_info, nullptr, &staging_buffer) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to create staging buffer");
                return;
            }

            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(device, staging_buffer, &mem_requirements);

            VkMemoryAllocateInfo alloc_info {};
            alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_requirements.size;
            alloc_info.memoryTypeIndex =
                find_memory_type(physical_device, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            VkDeviceMemory staging_memory;
            if (vkAllocateMemory(device, &alloc_info, nullptr, &staging_memory) != VK_SUCCESS)
            {
                vkDestroyBuffer(device, staging_buffer, nullptr);
                LOG_ERROR("Failed to allocate staging buffer memory");
                return;
            }

            vkBindBufferMemory(device, staging_buffer, staging_memory, 0);

            // 映射并复制数据
            void* mapped_data;
            vkMapMemory(device, staging_memory, 0, size, 0, &mapped_data);
            memcpy(mapped_data, data, size);
            vkUnmapMemory(device, staging_memory);

            // 复制缓冲区
            copyBuffer(device, command_pool, queue, staging_buffer, dst_buffer, size, 0, offset);

            // 清理
            vkDestroyBuffer(device, staging_buffer, nullptr);
            vkFreeMemory(device, staging_memory, nullptr);
        }

        void uploadDataDirect(VkDevice device, VkDeviceMemory memory, void* data, size_t size, size_t offset)
        {
            void* mapped_data;
            vkMapMemory(device, memory, offset, size, 0, &mapped_data);
            memcpy(mapped_data, data, size);
            vkUnmapMemory(device, memory);
        }

        // 缓冲区复制
        void copyBuffer(
            VkDevice      device,
            VkCommandPool command_pool,
            VkQueue       queue,
            VkBuffer      src_buffer,
            VkBuffer      dst_buffer,
            size_t        size,
            VkDeviceSize  src_offset,
            VkDeviceSize  dst_offset
        )
        {
            VkCommandBufferAllocateInfo alloc_info {};
            alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandPool        = command_pool;
            alloc_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

            VkCommandBufferBeginInfo begin_info {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(command_buffer, &begin_info);

            VkBufferCopy copy_region {};
            copy_region.srcOffset = src_offset;
            copy_region.dstOffset = dst_offset;
            copy_region.size      = size;
            vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info {};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &command_buffer;

            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);

            vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
        }

        // 缓冲区到图像复制
        void copyBufferToImage(
            VkDevice      device,
            VkCommandPool command_pool,
            VkQueue       queue,
            VkBuffer      src_buffer,
            VkImage       dst_image,
            VkExtent3D    extent,
            uint32_t      layer_count,
            uint32_t      mip_level
        )
        {
            VkCommandBufferAllocateInfo alloc_info {};
            alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandPool        = command_pool;
            alloc_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

            VkCommandBufferBeginInfo begin_info {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(command_buffer, &begin_info);

            VkBufferImageCopy region {};
            region.bufferOffset                    = 0;
            region.bufferRowLength                 = 0;
            region.bufferImageHeight               = 0;
            region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel       = mip_level;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount     = layer_count;
            region.imageOffset                     = {0, 0, 0};
            region.imageExtent                     = extent;

            vkCmdCopyBufferToImage(command_buffer, src_buffer, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info {};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &command_buffer;

            vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);

            vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
        }

        // 内存管理工具
        size_t alignSize(size_t size, size_t alignment) { return (size + alignment - 1) & ~(alignment - 1); }

        VkDeviceSize getBufferAlignment(VkPhysicalDevice physical_device, VkBufferUsageFlags usage_flags)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_device, &properties);

            if (usage_flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            {
                return properties.limits.minUniformBufferOffsetAlignment;
            }
            else if (usage_flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            {
                return properties.limits.minStorageBufferOffsetAlignment;
            }

            return 1;
        }

        VkDeviceSize getImageAlignment(VkPhysicalDevice physical_device, VkFormat /*format*/)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_device, &properties);
            return properties.limits.minTexelBufferOffsetAlignment;
        }

        // 内存映射工具
        void* mapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
        {
            void* data;
            if (vkMapMemory(device, memory, offset, size, 0, &data) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to map memory");
                return nullptr;
            }
            return data;
        }

        void unmapMemory(VkDevice device, VkDeviceMemory memory) { vkUnmapMemory(device, memory); }

        void flushMappedMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
        {
            VkMappedMemoryRange mapped_range {};
            mapped_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mapped_range.memory = memory;
            mapped_range.offset = offset;
            mapped_range.size   = size;

            vkFlushMappedMemoryRanges(device, 1, &mapped_range);
        }

        void invalidateMappedMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size)
        {
            VkMappedMemoryRange mapped_range {};
            mapped_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mapped_range.memory = memory;
            mapped_range.offset = offset;
            mapped_range.size   = size;

            vkInvalidateMappedMemoryRanges(device, 1, &mapped_range);
        }

        // 缓冲区验证和调试
        bool validateBuffer(VkDevice device, VkBuffer buffer)
        {
            if (buffer == VK_NULL_HANDLE)
            {
                LOG_ERROR("Buffer is null");
                return false;
            }

            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

            if (mem_requirements.size == 0)
            {
                LOG_ERROR("Buffer has zero size");
                return false;
            }

            return true;
        }

        void printBufferInfo(VkDevice device, VkBuffer buffer, const std::string& name)
        {
            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

            std::stringstream ss;
            ss << "Buffer Info";
            if (!name.empty())
            {
                ss << " [" << name << "]";
            }
            ss << ":\n";
            ss << "  Size: " << mem_requirements.size << " bytes\n";
            ss << "  Alignment: " << mem_requirements.alignment << " bytes\n";
            ss << "  Memory Type Bits: 0x" << std::hex << mem_requirements.memoryTypeBits << std::dec;

            LOG_INFO(ss.str());
        }

        void printMemoryInfo(VkPhysicalDevice physical_device, VkDeviceMemory /*memory*/, const std::string& name)
        {
            VkPhysicalDeviceMemoryProperties mem_properties;
            vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

            std::stringstream ss;
            ss << "Memory Info";
            if (!name.empty())
            {
                ss << " [" << name << "]";
            }
            ss << ":\n";
            ss << "  Memory Types: " << mem_properties.memoryTypeCount << "\n";
            ss << "  Memory Heaps: " << mem_properties.memoryHeapCount;

            LOG_INFO(ss.str());
        }

        // 高级缓冲区操作
        void updateBufferData(VkDevice device, VkDeviceMemory memory, void* data, size_t size, size_t offset)
        {
            void* mapped_data = mapMemory(device, memory, offset, size);
            if (mapped_data)
            {
                memcpy(mapped_data, data, size);
                unmapMemory(device, memory);
            }
        }

        void readBufferData(VkDevice device, VkDeviceMemory memory, void* data, size_t size, size_t offset)
        {
            void* mapped_data = mapMemory(device, memory, offset, size);
            if (mapped_data)
            {
                memcpy(data, mapped_data, size);
                unmapMemory(device, memory);
            }
        }

        // BufferPool 实现
        BufferPool::BufferPool(VkDevice device, VkPhysicalDevice physical_device, size_t buffer_size, VkBufferUsageFlags usage_flags)
            : m_device(device)
            , m_physical_device(physical_device)
            , m_buffer_size(buffer_size)
            , m_usage_flags(usage_flags)
        {}

        BufferPool::~BufferPool() { cleanup(); }

        VkBuffer BufferPool::allocateBuffer()
        {
            // 查找空闲缓冲区
            for (auto& buffer : m_buffers)
            {
                if (!buffer.in_use)
                {
                    buffer.in_use = true;
                    return buffer.buffer;
                }
            }

            // 创建新缓冲区
            VkBuffer buffer = createBuffer(m_device, m_buffer_size, m_usage_flags);
            if (buffer == VK_NULL_HANDLE)
            {
                return VK_NULL_HANDLE;
            }

            VkDeviceMemory memory = allocateBufferMemory(m_device, m_physical_device, buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (memory == VK_NULL_HANDLE)
            {
                vkDestroyBuffer(m_device, buffer, nullptr);
                return VK_NULL_HANDLE;
            }

            bindBufferMemory(m_device, buffer, memory);

            m_buffers.push_back({buffer, memory, true});
            return buffer;
        }

        void BufferPool::freeBuffer(VkBuffer buffer)
        {
            for (auto& pooled_buffer : m_buffers)
            {
                if (pooled_buffer.buffer == buffer)
                {
                    pooled_buffer.in_use = false;
                    break;
                }
            }
        }

        void BufferPool::cleanup()
        {
            for (const auto& pooled_buffer : m_buffers)
            {
                vkDestroyBuffer(m_device, pooled_buffer.buffer, nullptr);
                vkFreeMemory(m_device, pooled_buffer.memory, nullptr);
            }
            m_buffers.clear();
            m_free_indices.clear();
        }

        // 批量操作工具
        void batchUploadBuffers(
            VkDevice                                                          device,
            VkPhysicalDevice                                                  physical_device,
            VkCommandPool                                                     command_pool,
            VkQueue                                                           queue,
            const std::vector<std::pair<VkBuffer, std::pair<void*, size_t>>>& uploads
        )
        {
            for (const auto& upload : uploads)
            {
                uploadDataToBuffer(device, physical_device, command_pool, queue, upload.first, upload.second.first, upload.second.second);
            }
        }

        void batchCopyBuffers(VkDevice device, VkCommandPool command_pool, VkQueue queue, const std::vector<std::tuple<VkBuffer, VkBuffer, size_t>>& copies)
        {
            for (const auto& copy : copies)
            {
                copyBuffer(device, command_pool, queue, std::get<0>(copy), std::get<1>(copy), std::get<2>(copy));
            }
        }

        // 性能优化工具
        void optimizeBufferLayout(VkPhysicalDevice /*physical_device*/, std::vector<VkBuffer>& buffers)
        {
            // 根据缓冲区大小和用途进行排序优化
            std::sort(buffers.begin(), buffers.end(), [](VkBuffer a, VkBuffer b) {
                // 这里需要设备句柄来获取内存要求，暂时使用简单排序
                return reinterpret_cast<uintptr_t>(a) > reinterpret_cast<uintptr_t>(b);
            });
        }

        size_t calculateOptimalStagingBufferSize(VkPhysicalDevice physical_device, size_t total_data_size)
        {
            // 根据设备属性和数据大小计算最优暂存缓冲区大小
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_device, &properties);

            // 使用设备限制的合理倍数
            size_t optimal_size = std::max(total_data_size, static_cast<size_t>(properties.limits.minMemoryMapAlignment * 1024));
            return optimal_size;
        }

        // 错误处理和验证
        bool checkBufferSupport(VkPhysicalDevice physical_device, VkBufferUsageFlags usage_flags)
        {
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(physical_device, &features);

            // 检查各种缓冲区使用标志的支持
            if (usage_flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT && !features.shaderStorageBufferArrayDynamicIndexing)
            {
                LOG_WARN("Storage buffer dynamic indexing not supported");
                return false;
            }

            return true;
        }

        std::string getBufferUsageString(VkBufferUsageFlags usage_flags)
        {
            std::stringstream ss;
            if (usage_flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
                ss << "TRANSFER_SRC ";
            if (usage_flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
                ss << "TRANSFER_DST ";
            if (usage_flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
                ss << "UNIFORM_TEXEL ";
            if (usage_flags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
                ss << "STORAGE_TEXEL ";
            if (usage_flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
                ss << "UNIFORM ";
            if (usage_flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
                ss << "STORAGE ";
            if (usage_flags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
                ss << "INDEX ";
            if (usage_flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
                ss << "VERTEX ";
            if (usage_flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
                ss << "INDIRECT ";
            return ss.str();
        }

        std::string getMemoryPropertyString(VkMemoryPropertyFlags property_flags)
        {
            std::stringstream ss;
            if (property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                ss << "DEVICE_LOCAL ";
            if (property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                ss << "HOST_VISIBLE ";
            if (property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                ss << "HOST_COHERENT ";
            if (property_flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
                ss << "HOST_CACHED ";
            if (property_flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
                ss << "LAZILY_ALLOCATED ";
            return ss.str();
        }
    } // namespace GPUBufferUtility
} // namespace Piccolo