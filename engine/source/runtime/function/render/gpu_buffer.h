#pragma once

#include <string>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    /**
     * @brief 缓冲区上传信息结构
     * 
     * 用于封装缓冲区上传操作所需的所有参数
     */
    struct BufferUploadInfo
    {
        VkBuffer       buffer;        ///< 目标缓冲区
        VkDeviceMemory memory;        ///< 缓冲区内存
        size_t         size;          ///< 数据大小
        size_t         offset;        ///< 偏移量
        void*          data;          ///< 源数据指针
        bool           use_staging;   ///< 是否使用暂存缓冲区
    };

    /**
     * @brief GPU缓冲区封装类
     * 
     * 提供Vulkan缓冲区的RAII封装，包括：
     * - 缓冲区的创建和销毁
     * - 内存分配和管理
     * - 数据上传和下载
     * - 内存映射操作
     * 
     * 支持移动语义，禁用拷贝语义以确保资源安全
     */
    class GPUBuffer
    {
    public:
        // ========== 构造和析构 ==========
        
        /**
         * @brief 默认构造函数
         * 
         * 创建一个无效的缓冲区对象，需要后续调用initialize()进行初始化
         */
        GPUBuffer();
        
        /**
         * @brief 带参数的构造函数
         * 
         * 直接创建并初始化缓冲区
         * 
         * @param device Vulkan设备
         * @param physical_device 物理设备
         * @param size 缓冲区大小（字节）
         * @param usage 缓冲区使用标志
         * @param memory_properties 内存属性标志
         */
        GPUBuffer(VkDevice device, VkPhysicalDevice physical_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties);

        /**
         * @brief 析构函数
         * 
         * 注意：不会自动销毁Vulkan资源，需要手动调用destroy()方法
         */
        ~GPUBuffer();

        // ========== 拷贝和移动语义 ==========
        
        /**
         * @brief 禁用拷贝构造
         * 
         * 防止意外的资源复制，确保资源安全
         */
        GPUBuffer(const GPUBuffer&) = delete;
        
        /**
         * @brief 禁用拷贝赋值
         * 
         * 防止意外的资源复制，确保资源安全
         */
        GPUBuffer& operator=(const GPUBuffer&) = delete;

        /**
         * @brief 移动构造函数
         * 
         * 转移资源所有权，原对象变为无效状态
         * 
         * @param other 要移动的缓冲区对象
         */
        GPUBuffer(GPUBuffer&& other) noexcept;
        
        /**
         * @brief 移动赋值操作符
         * 
         * 转移资源所有权，原对象变为无效状态
         * 
         * @param other 要移动的缓冲区对象
         * @return 当前对象的引用
         */
        GPUBuffer& operator=(GPUBuffer&& other) noexcept;

        // ========== 缓冲区操作 ==========
        
        /**
         * @brief 初始化缓冲区
         * 
         * 创建Vulkan缓冲区和分配内存
         * 
         * @param device Vulkan设备
         * @param physical_device 物理设备
         * @param size 缓冲区大小（字节）
         * @param usage 缓冲区使用标志
         * @param memory_properties 内存属性标志
         * @return true表示成功，false表示失败
         */
        bool initialize(VkDevice device, VkPhysicalDevice physical_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_properties);

        /**
         * @brief 上传数据到缓冲区
         * 
         * 根据内存属性自动选择最佳的上传方式：
         * - 主机可见内存：直接映射上传
         * - 设备本地内存：使用暂存缓冲区上传
         * 
         * @param device Vulkan设备
         * @param physical_device 物理设备
         * @param command_pool 命令池
         * @param queue 队列
         * @param data 源数据指针
         * @param size 数据大小
         * @param offset 缓冲区偏移量
         * @return true表示成功，false表示失败
         */
        bool uploadData(VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue queue, void* data, size_t size, size_t offset = 0);

        /**
         * @brief 直接上传数据（适用于主机可见内存）
         * 
         * 通过内存映射直接上传数据，适用于频繁更新的缓冲区
         * 
         * @param device Vulkan设备
         * @param data 源数据指针
         * @param size 数据大小
         * @param offset 缓冲区偏移量
         * @return true表示成功，false表示失败
         */
        bool uploadDataDirect(VkDevice device, void* data, size_t size, size_t offset = 0);

        /**
         * @brief 从缓冲区读取数据
         * 
         * 从主机可见的缓冲区内存中读取数据
         * 
         * @param device Vulkan设备
         * @param data 目标数据指针
         * @param size 数据大小
         * @param offset 缓冲区偏移量
         * @return true表示成功，false表示失败
         */
        bool readData(VkDevice device, void* data, size_t size, size_t offset = 0);

        // ========== 内存映射操作 ==========
        
        /**
         * @brief 映射缓冲区内存
         * 
         * 将缓冲区内存映射到主机地址空间
         * 
         * @param device Vulkan设备
         * @param offset 内存偏移量
         * @param size 映射大小，VK_WHOLE_SIZE表示映射全部
         * @return 映射的内存指针，失败返回nullptr
         */
        void* mapMemory(VkDevice device, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
        
        /**
         * @brief 取消内存映射
         * 
         * @param device Vulkan设备
         */
        void unmapMemory(VkDevice device);

        /**
         * @brief 刷新映射的内存
         * 
         * 确保主机写入的数据对设备可见
         * 
         * @param device Vulkan设备
         * @param offset 内存偏移量
         * @param size 刷新大小
         */
        void flushMappedMemory(VkDevice device, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

        // ========== 资源管理 ==========
        
        /**
         * @brief 销毁缓冲区
         * 
         * 释放Vulkan缓冲区和内存资源
         * 
         * @param device Vulkan设备
         */
        void destroy(VkDevice device);

        // ========== 访问器方法 ==========
        
        /**
         * @brief 获取Vulkan缓冲区句柄
         * @return VkBuffer句柄
         */
        VkBuffer getBuffer() const { return m_buffer; }
        
        /**
         * @brief 获取设备内存句柄
         * @return VkDeviceMemory句柄
         */
        VkDeviceMemory getMemory() const { return m_memory; }
        
        /**
         * @brief 获取缓冲区大小
         * @return 缓冲区大小（字节）
         */
        size_t getSize() const { return m_size; }
        
        /**
         * @brief 获取缓冲区使用标志
         * @return VkBufferUsageFlags
         */
        VkBufferUsageFlags getUsage() const { return m_usage; }
        
        /**
         * @brief 获取内存属性标志
         * @return VkMemoryPropertyFlags
         */
        VkMemoryPropertyFlags getMemoryProperties() const { return m_memory_properties; }
        
        /**
         * @brief 检查缓冲区是否有效
         * @return true表示缓冲区有效，false表示无效
         */
        bool isValid() const { return m_buffer != VK_NULL_HANDLE && m_memory != VK_NULL_HANDLE; }

    private:
        // ========== 成员变量 ==========
        
        VkBuffer              m_buffer;            ///< Vulkan缓冲区句柄
        VkDeviceMemory        m_memory;            ///< 设备内存句柄
        size_t                m_size;              ///< 缓冲区大小（字节）
        VkBufferUsageFlags    m_usage;             ///< 缓冲区使用标志
        VkMemoryPropertyFlags m_memory_properties; ///< 内存属性标志
        bool                  m_is_mapped;         ///< 内存是否已映射
    };

    /**
     * @brief GPU缓冲区池管理类
     * 
     * 提供缓冲区的池化管理，用于减少频繁的缓冲区创建和销毁开销
     * 适用于需要大量相同大小和用途缓冲区的场景
     */
    class GPUBufferPool
    {
    public:
        /**
         * @brief 构造函数
         * 
         * @param device Vulkan设备
         * @param physical_device 物理设备
         * @param buffer_size 池中缓冲区的大小
         * @param usage_flags 缓冲区的使用标志
         */
        GPUBufferPool(VkDevice device, VkPhysicalDevice physical_device, size_t buffer_size, VkBufferUsageFlags usage_flags);
        
        /**
         * @brief 析构函数
         * 
         * 自动清理所有池中的缓冲区
         */
        ~GPUBufferPool();

        /**
         * @brief 分配缓冲区
         * 
         * 从池中分配一个可用的缓冲区，如果没有可用缓冲区则创建新的
         * 
         * @return 分配的缓冲区句柄，失败返回VK_NULL_HANDLE
         */
        VkBuffer allocateBuffer();
        
        /**
         * @brief 释放缓冲区
         * 
         * 将缓冲区标记为可用，返回池中供后续分配
         * 
         * @param buffer 要释放的缓冲区句柄
         */
        void freeBuffer(VkBuffer buffer);
        
        /**
         * @brief 清理池
         * 
         * 销毁池中的所有缓冲区
         */
        void cleanup();

    private:
        // ========== 成员变量 ==========
        
        VkDevice           m_device;           ///< Vulkan设备
        VkPhysicalDevice   m_physical_device;  ///< 物理设备
        size_t             m_buffer_size;      ///< 池中缓冲区的大小
        VkBufferUsageFlags m_usage_flags;      ///< 缓冲区使用标志

        /**
         * @brief 池化缓冲区结构
         */
        struct PooledBuffer
        {
            VkBuffer       buffer;  ///< 缓冲区句柄
            VkDeviceMemory memory;  ///< 内存句柄
            bool           in_use;  ///< 是否正在使用
        };

        std::vector<PooledBuffer> m_buffers;    ///< 池中的缓冲区列表
        std::vector<size_t>       m_free_indices; ///< 空闲缓冲区的索引列表
    };
} // namespace Piccolo
