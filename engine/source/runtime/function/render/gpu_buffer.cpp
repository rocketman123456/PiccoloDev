#include "gpu_buffer.h"
#include "utils/gpu_buffer_utils.h"

#include "runtime/core/base/macro.h"

namespace Piccolo
{
    // GPUBuffer 类实现
    GPUBuffer::GPUBuffer()
        : m_buffer(VK_NULL_HANDLE)
        , m_memory(VK_NULL_HANDLE)
        , m_size(0)
        , m_usage(0)
        , m_memory_properties(0)
        , m_is_mapped(false)
    {}

    GPUBuffer::GPUBuffer(VkDevice device, VkPhysicalDevice physical_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties)
        : m_buffer(VK_NULL_HANDLE)
        , m_memory(VK_NULL_HANDLE)
        , m_size(0)
        , m_usage(0)
        , m_memory_properties(0)
        , m_is_mapped(false)
    {
        initialize(device, physical_device, size, usage, memory_properties);
    }

    GPUBuffer::~GPUBuffer()
    {
        // 注意：析构函数中不能调用destroy，因为需要VkDevice参数
        // 用户需要手动调用destroy()方法
    }

    GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
        : m_buffer(other.m_buffer)
        , m_memory(other.m_memory)
        , m_size(other.m_size)
        , m_usage(other.m_usage)
        , m_memory_properties(other.m_memory_properties)
        , m_is_mapped(other.m_is_mapped)
    {
        other.m_buffer            = VK_NULL_HANDLE;
        other.m_memory            = VK_NULL_HANDLE;
        other.m_size              = 0;
        other.m_usage             = 0;
        other.m_memory_properties = 0;
        other.m_is_mapped         = false;
    }

    GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
    {
        if (this != &other)
        {
            m_buffer            = other.m_buffer;
            m_memory            = other.m_memory;
            m_size              = other.m_size;
            m_usage             = other.m_usage;
            m_memory_properties = other.m_memory_properties;
            m_is_mapped         = other.m_is_mapped;

            other.m_buffer            = VK_NULL_HANDLE;
            other.m_memory            = VK_NULL_HANDLE;
            other.m_size              = 0;
            other.m_usage             = 0;
            other.m_memory_properties = 0;
            other.m_is_mapped         = false;
        }
        return *this;
    }

    bool
    GPUBuffer::initialize(VkDevice device, VkPhysicalDevice physical_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties)
    {
        if (isValid())
        {
            LOG_WARN("GPUBuffer already initialized");
            return false;
        }

        m_size              = size;
        m_usage             = usage;
        m_memory_properties = memory_properties;

        // 创建缓冲区
        m_buffer = GPUBufferUtility::createBuffer(device, size, usage);
        if (m_buffer == VK_NULL_HANDLE)
        {
            LOG_ERROR("Failed to create buffer");
            return false;
        }

        // 分配内存
        m_memory = GPUBufferUtility::allocateBufferMemory(device, physical_device, m_buffer, memory_properties);
        if (m_memory == VK_NULL_HANDLE)
        {
            LOG_ERROR("Failed to allocate buffer memory");
            vkDestroyBuffer(device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
            return false;
        }

        // 绑定内存
        GPUBufferUtility::bindBufferMemory(device, m_buffer, m_memory);

        return true;
    }

    bool
    GPUBuffer::uploadData(VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue queue, void* data, size_t size, size_t offset)
    {
        if (!isValid())
        {
            LOG_ERROR("Buffer is not valid");
            return false;
        }

        if (offset + size > m_size)
        {
            LOG_ERROR("Upload size exceeds buffer size");
            return false;
        }

        // 如果内存是主机可见的，直接上传
        if (m_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            return uploadDataDirect(device, data, size, offset);
        }
        else
        {
            // 使用暂存缓冲区上传
            GPUBufferUtility::uploadDataWithStagingBuffer(device, physical_device, command_pool, queue, m_buffer, data, size, offset);
            return true;
        }
    }

    bool GPUBuffer::uploadDataDirect(VkDevice device, void* data, size_t size, size_t offset)
    {
        if (!isValid())
        {
            LOG_ERROR("Buffer is not valid");
            return false;
        }

        if (!(m_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            LOG_ERROR("Buffer memory is not host visible");
            return false;
        }

        if (offset + size > m_size)
        {
            LOG_ERROR("Upload size exceeds buffer size");
            return false;
        }

        GPUBufferUtility::uploadDataDirect(device, m_memory, data, size, offset);
        return true;
    }

    bool GPUBuffer::readData(VkDevice device, void* data, size_t size, size_t offset)
    {
        if (!isValid())
        {
            LOG_ERROR("Buffer is not valid");
            return false;
        }

        if (!(m_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            LOG_ERROR("Buffer memory is not host visible");
            return false;
        }

        if (offset + size > m_size)
        {
            LOG_ERROR("Read size exceeds buffer size");
            return false;
        }

        GPUBufferUtility::readBufferData(device, m_memory, data, size, offset);
        return true;
    }

    void* GPUBuffer::mapMemory(VkDevice device, VkDeviceSize offset, VkDeviceSize size)
    {
        if (!isValid())
        {
            LOG_ERROR("Buffer is not valid");
            return nullptr;
        }

        if (!(m_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            LOG_ERROR("Buffer memory is not host visible");
            return nullptr;
        }

        if (m_is_mapped)
        {
            LOG_WARN("Buffer memory is already mapped");
            return nullptr;
        }

        void* data = GPUBufferUtility::mapMemory(device, m_memory, offset, size);
        if (data)
        {
            m_is_mapped = true;
        }
        return data;
    }

    void GPUBuffer::unmapMemory(VkDevice device)
    {
        if (m_is_mapped)
        {
            GPUBufferUtility::unmapMemory(device, m_memory);
            m_is_mapped = false;
        }
    }

    void GPUBuffer::flushMappedMemory(VkDevice device, VkDeviceSize offset, VkDeviceSize size)
    {
        if (m_is_mapped)
        {
            GPUBufferUtility::flushMappedMemory(device, m_memory, offset, size);
        }
    }

    void GPUBuffer::destroy(VkDevice device)
    {
        if (m_is_mapped)
        {
            unmapMemory(device);
        }

        if (m_buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }

        m_size              = 0;
        m_usage             = 0;
        m_memory_properties = 0;
    }

    // GPUBufferPool 实现
    GPUBufferPool::GPUBufferPool(VkDevice device, VkPhysicalDevice physical_device, size_t buffer_size, VkBufferUsageFlags usage_flags)
        : m_device(device)
        , m_physical_device(physical_device)
        , m_buffer_size(buffer_size)
        , m_usage_flags(usage_flags)
    {}

    GPUBufferPool::~GPUBufferPool() { cleanup(); }

    VkBuffer GPUBufferPool::allocateBuffer()
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
        VkBuffer buffer = GPUBufferUtility::createBuffer(m_device, m_buffer_size, m_usage_flags);
        if (buffer == VK_NULL_HANDLE)
        {
            return VK_NULL_HANDLE;
        }

        VkDeviceMemory memory = GPUBufferUtility::allocateBufferMemory(m_device, m_physical_device, buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memory == VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_device, buffer, nullptr);
            return VK_NULL_HANDLE;
        }

        GPUBufferUtility::bindBufferMemory(m_device, buffer, memory);

        m_buffers.push_back({buffer, memory, true});
        return buffer;
    }

    void GPUBufferPool::freeBuffer(VkBuffer buffer)
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

    void GPUBufferPool::cleanup()
    {
        for (const auto& pooled_buffer : m_buffers)
        {
            vkDestroyBuffer(m_device, pooled_buffer.buffer, nullptr);
            vkFreeMemory(m_device, pooled_buffer.memory, nullptr);
        }
        m_buffers.clear();
        m_free_indices.clear();
    }
} // namespace Piccolo
