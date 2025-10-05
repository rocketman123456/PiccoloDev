#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/render_type.h"

#include <memory>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace Piccolo
{
    /**
     * @brief 资源句柄类型
     */
    using ResourceHandle = uint32_t;

    /**
     * @brief 无效资源句柄
     */
    constexpr ResourceHandle INVALID_RESOURCE_HANDLE = 0;

    /**
     * @brief 资源池基类
     * 提供资源池化的基础功能
     */
    template<typename T>
    class ResourcePool
    {
    public:
        /**
         * @brief 构造函数
         */
        ResourcePool() = default;

        /**
         * @brief 析构函数
         */
        ~ResourcePool() = default;

        /**
         * @brief 获取资源
         * @return 资源的指针，如果池为空则返回nullptr
         */
        T* acquire()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_available.empty()) {
                return nullptr;
            }
            
            T* resource = m_available.front();
            m_available.pop();
            return resource;
        }

        /**
         * @brief 释放资源
         * @param resource 要释放的资源指针
         */
        void release(T* resource)
        {
            if (!resource) return;
            
            std::lock_guard<std::mutex> lock(m_mutex);
            m_available.push(resource);
        }

        /**
         * @brief 添加资源到池中
         * @param resource 要添加的资源
         */
        void addResource(std::unique_ptr<T> resource)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pool.push_back(std::move(resource));
            m_available.push(m_pool.back().get());
        }

        /**
         * @brief 清空资源池
         */
        void clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!m_available.empty()) {
                m_available.pop();
            }
            m_pool.clear();
        }

        /**
         * @brief 获取池中资源数量
         * @return 资源数量
         */
        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_pool.size();
        }

        /**
         * @brief 获取可用资源数量
         * @return 可用资源数量
         */
        size_t availableCount() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_available.size();
        }

    private:
        mutable std::mutex m_mutex;                    ///< 互斥锁
        std::vector<std::unique_ptr<T>> m_pool;        ///< 资源池
        std::queue<T*> m_available;                    ///< 可用资源队列
    };

    /**
     * @brief 纹理资源信息
     */
    struct TextureResource
    {
        RHIImage* image = nullptr;                     ///< 图像对象
        RHIImageView* image_view = nullptr;            ///< 图像视图
        RHISampler* sampler = nullptr;                 ///< 采样器
        VmaAllocation allocation = nullptr;            ///< VMA分配
        RHIFormat format = RHI_FORMAT_MAX_ENUM;        ///< 格式
        uint32_t width = 0;                            ///< 宽度
        uint32_t height = 0;                           ///< 高度
        uint32_t mip_levels = 1;                       ///< Mip级别数
        bool is_valid = false;                         ///< 是否有效

        /**
         * @brief 构造函数
         */
        TextureResource() = default;

        /**
         * @brief 析构函数
         */
        ~TextureResource() = default;

        /**
         * @brief 重置资源
         */
        void reset()
        {
            image = nullptr;
            image_view = nullptr;
            sampler = nullptr;
            allocation = nullptr;
            format = RHI_FORMAT_MAX_ENUM;
            width = 0;
            height = 0;
            mip_levels = 1;
            is_valid = false;
        }
    };

    /**
     * @brief 缓冲区资源信息
     */
    struct BufferResource
    {
        RHIBuffer* buffer = nullptr;                   ///< 缓冲区对象
        VmaAllocation allocation = nullptr;            ///< VMA分配
        RHIDeviceSize size = 0;                        ///< 大小
        RHIBufferUsageFlags usage = 0;                 ///< 使用标志
        bool is_valid = false;                         ///< 是否有效

        /**
         * @brief 构造函数
         */
        BufferResource() = default;

        /**
         * @brief 析构函数
         */
        ~BufferResource() = default;

        /**
         * @brief 重置资源
         */
        void reset()
        {
            buffer = nullptr;
            allocation = nullptr;
            size = 0;
            usage = 0;
            is_valid = false;
        }
    };

    /**
     * @brief 纹理资源池
     * 管理纹理资源的分配和回收
     */
    class TexturePool
    {
    public:
        /**
         * @brief 构造函数
         * @param rhi 渲染硬件接口
         */
        explicit TexturePool(std::shared_ptr<RHI> rhi);

        /**
         * @brief 析构函数
         */
        ~TexturePool();

        /**
         * @brief 创建纹理
         * @param info 纹理创建信息
         * @return 纹理句柄
         */
        ResourceHandle createTexture(const RHIImageCreateInfo& info);

        /**
         * @brief 销毁纹理
         * @param handle 纹理句柄
         */
        void destroyTexture(ResourceHandle handle);

        /**
         * @brief 获取纹理资源
         * @param handle 纹理句柄
         * @return 纹理资源指针
         */
        TextureResource* getTexture(ResourceHandle handle);

        /**
         * @brief 获取纹理资源（常量版本）
         * @param handle 纹理句柄
         * @return 纹理资源指针
         */
        const TextureResource* getTexture(ResourceHandle handle) const;

        /**
         * @brief 清空纹理池
         */
        void clear();

        /**
         * @brief 获取纹理数量
         * @return 纹理数量
         */
        size_t getTextureCount() const { return m_textures.size(); }

        /**
         * @brief 获取可用纹理数量
         * @return 可用纹理数量
         */
        size_t getAvailableTextureCount() const { return m_texture_pool.availableCount(); }

    private:
        std::shared_ptr<RHI> m_rhi;                                    ///< 渲染硬件接口
        ResourcePool<TextureResource> m_texture_pool;                  ///< 纹理资源池
        std::unordered_map<ResourceHandle, std::unique_ptr<TextureResource>> m_textures; ///< 纹理资源映射
        ResourceHandle m_next_handle = 1;                              ///< 下一个句柄ID
        mutable std::mutex m_mutex;                                    ///< 互斥锁

        /**
         * @brief 分配新的句柄
         * @return 新的句柄
         */
        ResourceHandle allocateHandle();
    };

    /**
     * @brief 缓冲区资源池
     * 管理缓冲区资源的分配和回收
     */
    class BufferPool
    {
    public:
        /**
         * @brief 构造函数
         * @param rhi 渲染硬件接口
         */
        explicit BufferPool(std::shared_ptr<RHI> rhi);

        /**
         * @brief 析构函数
         */
        ~BufferPool();

        /**
         * @brief 创建缓冲区
         * @param info 缓冲区创建信息
         * @return 缓冲区句柄
         */
        ResourceHandle createBuffer(const RHIBufferCreateInfo& info);

        /**
         * @brief 销毁缓冲区
         * @param handle 缓冲区句柄
         */
        void destroyBuffer(ResourceHandle handle);

        /**
         * @brief 获取缓冲区资源
         * @param handle 缓冲区句柄
         * @return 缓冲区资源指针
         */
        BufferResource* getBuffer(ResourceHandle handle);

        /**
         * @brief 获取缓冲区资源（常量版本）
         * @param handle 缓冲区句柄
         * @return 缓冲区资源指针
         */
        const BufferResource* getBuffer(ResourceHandle handle) const;

        /**
         * @brief 清空缓冲区池
         */
        void clear();

        /**
         * @brief 获取缓冲区数量
         * @return 缓冲区数量
         */
        size_t getBufferCount() const { return m_buffers.size(); }

        /**
         * @brief 获取可用缓冲区数量
         * @return 可用缓冲区数量
         */
        size_t getAvailableBufferCount() const { return m_buffer_pool.availableCount(); }

    private:
        std::shared_ptr<RHI> m_rhi;                                    ///< 渲染硬件接口
        ResourcePool<BufferResource> m_buffer_pool;                    ///< 缓冲区资源池
        std::unordered_map<ResourceHandle, std::unique_ptr<BufferResource>> m_buffers; ///< 缓冲区资源映射
        ResourceHandle m_next_handle = 1;                              ///< 下一个句柄ID
        mutable std::mutex m_mutex;                                    ///< 互斥锁

        /**
         * @brief 分配新的句柄
         * @return 新的句柄
         */
        ResourceHandle allocateHandle();
    };

    /**
     * @brief 渲染资源池管理器
     * 统一管理所有类型的资源池
     */
    class RenderResourcePoolManager
    {
    public:
        /**
         * @brief 构造函数
         * @param rhi 渲染硬件接口
         */
        explicit RenderResourcePoolManager(std::shared_ptr<RHI> rhi);

        /**
         * @brief 析构函数
         */
        ~RenderResourcePoolManager();

        // ========== 资源池访问接口 ==========

        /**
         * @brief 获取纹理池
         * @return 纹理池的引用
         */
        TexturePool& getTexturePool() { return m_texture_pool; }

        /**
         * @brief 获取缓冲区池
         * @return 缓冲区池的引用
         */
        BufferPool& getBufferPool() { return m_buffer_pool; }

        // ========== 统一管理接口 ==========

        /**
         * @brief 初始化资源池管理器
         * @return 是否初始化成功
         */
        bool initialize();

        /**
         * @brief 清理资源池管理器
         */
        void cleanup();

        /**
         * @brief 获取资源统计信息
         * @return 统计信息字符串
         */
        std::string getResourceStatistics() const;

        /**
         * @brief 清空所有资源池
         */
        void clearAllPools();

    private:
        std::shared_ptr<RHI> m_rhi;        ///< 渲染硬件接口
        TexturePool m_texture_pool;        ///< 纹理池
        BufferPool m_buffer_pool;          ///< 缓冲区池
        bool m_initialized = false;        ///< 是否已初始化
    };
} // namespace Piccolo
