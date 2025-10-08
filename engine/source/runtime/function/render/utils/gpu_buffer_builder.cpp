#include "runtime/function/render/utils/gpu_buffer_builder.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/log/log_system.h"

#include <stdexcept>
#include <cstring>

namespace Piccolo
{
    // GPUBufferBuilder 实现
    GPUBufferBuilder::GPUBufferBuilder(VkDevice device, VkPhysicalDevice physical_device)
        : m_device(device), m_physical_device(physical_device)
    {
        reset();
    }

    GPUBufferBuilder& GPUBufferBuilder::setName(const std::string& name)
    {
        m_config.name = name;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setDescription(const std::string& description)
    {
        m_config.description = description;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setSize(size_t size)
    {
        m_config.size = size;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setInitialData(void* data, size_t data_size)
    {
        m_config.initial_data = data;
        if (data_size > 0 && data_size != m_config.size)
        {
            m_config.size = data_size;
        }
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setType(BufferType type)
    {
        m_config.type = type;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setUsage(BufferUsage usage)
    {
        m_config.usage = usage;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setMemoryType(BufferMemoryType memory_type)
    {
        m_config.memory_type = memory_type;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setUsageFlags(VkBufferUsageFlags flags)
    {
        m_config.usage_flags = flags;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setMemoryPropertyFlags(VkMemoryPropertyFlags flags)
    {
        m_config.memory_property_flags = flags;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setPersistent(bool persistent)
    {
        m_config.persistent = persistent;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setCoherent(bool coherent)
    {
        m_config.coherent = coherent;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::setCached(bool cached)
    {
        m_config.cached = cached;
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::asVertexBuffer(size_t size)
    {
        m_config.type = BufferType::Vertex;
        m_config.size = size;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::asIndexBuffer(size_t size)
    {
        m_config.type = BufferType::Index;
        m_config.size = size;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::asUniformBuffer(size_t size)
    {
        m_config.type = BufferType::Uniform;
        m_config.size = size;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::asStorageBuffer(size_t size)
    {
        m_config.type = BufferType::Storage;
        m_config.size = size;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::asStagingBuffer(size_t size)
    {
        m_config.type = BufferType::Staging;
        m_config.size = size;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferBuilder& GPUBufferBuilder::asIndirectBuffer(size_t size)
    {
        m_config.type = BufferType::Indirect;
        m_config.size = size;
        setupDefaultFlags();
        return *this;
    }

    GPUBufferInfo GPUBufferBuilder::build()
    {
        validateConfig();

        GPUBufferInfo buffer_info;
        buffer_info.size = m_config.size;

        // 创建缓冲区
        VkBufferCreateInfo buffer_info_vk {};
        buffer_info_vk.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info_vk.size = m_config.size;
        buffer_info_vk.usage = m_config.usage_flags;
        buffer_info_vk.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device, &buffer_info_vk, nullptr, &buffer_info.buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer: " + m_config.name);
        }

        // 获取内存要求
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(m_device, buffer_info.buffer, &mem_requirements);
        buffer_info.alignment = mem_requirements.alignment;

        // 分配内存
        VkMemoryAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, m_config.memory_property_flags);

        if (vkAllocateMemory(m_device, &alloc_info, nullptr, &buffer_info.memory) != VK_SUCCESS)
        {
            vkDestroyBuffer(m_device, buffer_info.buffer, nullptr);
            throw std::runtime_error("Failed to allocate buffer memory: " + m_config.name);
        }

        // 绑定内存
        vkBindBufferMemory(m_device, buffer_info.buffer, buffer_info.memory, 0);

        // 如果有初始数据，复制到缓冲区
        if (m_config.initial_data)
        {
            copyDataToBuffer(buffer_info.buffer, m_config.initial_data, m_config.size);
        }

        // 如果需要持久化映射
        if (m_config.persistent)
        {
            buffer_info.mapped_memory = GPUBufferUtils::mapBuffer(m_device, buffer_info.memory);
            buffer_info.is_mapped = true;
        }

        LOG_INFO("Created buffer: {} (size: {} bytes)", m_config.name, m_config.size);
        return buffer_info;
    }

    GPUBufferInfo GPUBufferBuilder::buildWithData(void* data, size_t data_size)
    {
        setInitialData(data, data_size);
        return build();
    }

    void GPUBufferBuilder::reset()
    {
        m_config = GPUBufferConfig();
        m_config.name = "UnnamedBuffer";
        m_config.description = "";
    }

    void GPUBufferBuilder::validateConfig() const
    {
        if (m_config.size == 0)
        {
            throw std::runtime_error("Buffer size cannot be zero: " + m_config.name);
        }

        if (m_config.usage_flags == 0)
        {
            throw std::runtime_error("Buffer usage flags cannot be zero: " + m_config.name);
        }

        if (m_config.memory_property_flags == 0)
        {
            throw std::runtime_error("Buffer memory property flags cannot be zero: " + m_config.name);
        }
    }

    void GPUBufferBuilder::setupDefaultFlags()
    {
        // 根据缓冲区类型设置默认使用标志
        switch (m_config.type)
        {
            case BufferType::Vertex:
                m_config.usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case BufferType::Index:
                m_config.usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case BufferType::Uniform:
                m_config.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                break;
            case BufferType::Storage:
                m_config.usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case BufferType::Staging:
                m_config.usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                break;
            case BufferType::Indirect:
                m_config.usage_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case BufferType::AccelerationStructure:
                m_config.usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
        }

        // 根据使用模式调整标志
        switch (m_config.usage)
        {
            case BufferUsage::Dynamic:
                m_config.usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case BufferUsage::Stream:
                m_config.usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            default:
                break;
        }

        // 根据内存类型设置内存属性标志
        switch (m_config.memory_type)
        {
            case BufferMemoryType::Device:
                m_config.memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            case BufferMemoryType::Host:
                m_config.memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            case BufferMemoryType::HostVisible:
                m_config.memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                if (m_config.coherent)
                {
                    m_config.memory_property_flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                }
                if (m_config.cached)
                {
                    m_config.memory_property_flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                }
                break;
        }
    }

    uint32_t GPUBufferBuilder::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        {
            if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type for buffer: " + m_config.name);
    }

    void GPUBufferBuilder::copyDataToBuffer(VkBuffer /*buffer*/, void* data, size_t data_size)
    {
        // 创建临时暂存缓冲区
        auto staging_buffer = GPUBufferUtils::createStagingBuffer(m_device, m_physical_device, data_size);
        
        // 映射暂存缓冲区并复制数据
        void* mapped_data = GPUBufferUtils::mapBuffer(m_device, staging_buffer.memory);
        memcpy(mapped_data, data, data_size);
        GPUBufferUtils::unmapBuffer(m_device, staging_buffer.memory);

        // 这里需要命令缓冲区来复制数据，暂时使用直接映射的方式
        // 在实际应用中，应该使用命令缓冲区进行异步复制
        LOG_WARN("Direct buffer copy not implemented, using staging buffer approach");
        
        // 清理暂存缓冲区
        GPUBufferUtils::destroyBuffer(m_device, staging_buffer);
    }

    // GPUBufferConfigFactory 实现
    GPUBufferConfig GPUBufferConfigFactory::createVertexBufferConfig(size_t size, bool dynamic)
    {
        GPUBufferConfig config;
        config.name = "VertexBuffer";
        config.type = BufferType::Vertex;
        config.size = size;
        config.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
        config.memory_type = dynamic ? BufferMemoryType::HostVisible : BufferMemoryType::Device;
        config.usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        config.memory_property_flags = dynamic ? 
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) :
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return config;
    }

    GPUBufferConfig GPUBufferConfigFactory::createIndexBufferConfig(size_t size, bool dynamic)
    {
        GPUBufferConfig config;
        config.name = "IndexBuffer";
        config.type = BufferType::Index;
        config.size = size;
        config.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
        config.memory_type = dynamic ? BufferMemoryType::HostVisible : BufferMemoryType::Device;
        config.usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        config.memory_property_flags = dynamic ? 
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) :
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return config;
    }

    GPUBufferConfig GPUBufferConfigFactory::createUniformBufferConfig(size_t size, bool dynamic)
    {
        GPUBufferConfig config;
        config.name = "UniformBuffer";
        config.type = BufferType::Uniform;
        config.size = size;
        config.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
        config.memory_type = BufferMemoryType::HostVisible;
        config.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        config.memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        return config;
    }

    GPUBufferConfig GPUBufferConfigFactory::createStorageBufferConfig(size_t size, bool dynamic)
    {
        GPUBufferConfig config;
        config.name = "StorageBuffer";
        config.type = BufferType::Storage;
        config.size = size;
        config.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
        config.memory_type = dynamic ? BufferMemoryType::HostVisible : BufferMemoryType::Device;
        config.usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        config.memory_property_flags = dynamic ? 
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) :
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return config;
    }

    GPUBufferConfig GPUBufferConfigFactory::createStagingBufferConfig(size_t size)
    {
        GPUBufferConfig config;
        config.name = "StagingBuffer";
        config.type = BufferType::Staging;
        config.size = size;
        config.usage = BufferUsage::Stream;
        config.memory_type = BufferMemoryType::Host;
        config.usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        config.memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        return config;
    }

    GPUBufferConfig GPUBufferConfigFactory::createIndirectBufferConfig(size_t size, bool dynamic)
    {
        GPUBufferConfig config;
        config.name = "IndirectBuffer";
        config.type = BufferType::Indirect;
        config.size = size;
        config.usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
        config.memory_type = dynamic ? BufferMemoryType::HostVisible : BufferMemoryType::Device;
        config.usage_flags = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        config.memory_property_flags = dynamic ? 
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) :
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return config;
    }

    GPUBufferConfig GPUBufferConfigFactory::createAccelerationStructureBufferConfig(size_t size)
    {
        GPUBufferConfig config;
        config.name = "AccelerationStructureBuffer";
        config.type = BufferType::AccelerationStructure;
        config.size = size;
        config.usage = BufferUsage::Static;
        config.memory_type = BufferMemoryType::Device;
        config.usage_flags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        config.memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        return config;
    }

    // GPUBufferUtils 实现
    GPUBufferInfo GPUBufferUtils::createStagingBuffer(VkDevice device, VkPhysicalDevice physical_device, size_t size)
    {
        GPUBufferBuilder builder(device, physical_device);
        auto config = GPUBufferConfigFactory::createStagingBufferConfig(size);
        builder.setName(config.name)
               .setSize(config.size)
               .setUsageFlags(config.usage_flags)
               .setMemoryPropertyFlags(config.memory_property_flags);
        
        return builder.build();
    }

    void GPUBufferUtils::copyDataToBuffer(VkDevice /*device*/, VkBuffer /*buffer*/, void* /*data*/, size_t /*data_size*/)
    {
        // 这里需要更复杂的实现来处理不同类型的缓冲区
        // 暂时记录警告
        LOG_WARN("GPUBufferUtils::copyDataToBuffer - 需要命令缓冲区实现");
    }

    void GPUBufferUtils::copyBuffer(VkDevice device, VkCommandPool command_pool, VkQueue queue, 
                                   VkBuffer src_buffer, VkBuffer dst_buffer, size_t size)
    {
        // 创建命令缓冲区
        VkCommandBufferAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = command_pool;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

        // 开始记录命令
        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(command_buffer, &begin_info);

        // 复制缓冲区
        VkBufferCopy copy_region {};
        copy_region.size = size;
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

        vkEndCommandBuffer(command_buffer);

        // 提交命令缓冲区
        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    }

    void* GPUBufferUtils::mapBuffer(VkDevice device, VkDeviceMemory memory, size_t offset, size_t size)
    {
        void* data;
        if (vkMapMemory(device, memory, offset, size, 0, &data) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map buffer memory");
        }
        return data;
    }

    void GPUBufferUtils::unmapBuffer(VkDevice device, VkDeviceMemory memory)
    {
        vkUnmapMemory(device, memory);
    }

    void GPUBufferUtils::flushMappedMemory(VkDevice device, VkDeviceMemory memory, size_t offset, size_t size)
    {
        VkMappedMemoryRange mapped_range {};
        mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.memory = memory;
        mapped_range.offset = offset;
        mapped_range.size = size;

        vkFlushMappedMemoryRanges(device, 1, &mapped_range);
    }

    void GPUBufferUtils::destroyBuffer(VkDevice device, const GPUBufferInfo& buffer_info)
    {
        if (buffer_info.is_mapped)
        {
            unmapBuffer(device, buffer_info.memory);
        }

        if (buffer_info.buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, buffer_info.buffer, nullptr);
        }

        if (buffer_info.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, buffer_info.memory, nullptr);
        }
    }

    size_t GPUBufferUtils::getBufferAlignment(VkPhysicalDevice physical_device, VkBufferUsageFlags usage_flags)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_device, &properties);

        // 根据使用标志返回适当的对齐
        if (usage_flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            return properties.limits.minUniformBufferOffsetAlignment;
        }
        else if (usage_flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            return properties.limits.minStorageBufferOffsetAlignment;
        }

        return 1; // 默认对齐
    }

} // namespace Piccolo
