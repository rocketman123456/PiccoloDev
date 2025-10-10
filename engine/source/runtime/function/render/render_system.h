#pragma once

#include "runtime/function/render/gpu_buffer.h"
#include "runtime/function/render/utils/gpu_buffer_utils.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <memory>
namespace Piccolo
{
    // 前向声明GPU相关类
    class GPUContext;
    class GPUDevice;
    class GPUSwapChain;
    class GPUPipeline;
    class GPURenderPass;
    class GPUCommandPool;
    class GPUSyncObject;
    class GPURenderResourceManager;
    class GPURenderStateManager;
    class GPUProfiler;
    class CPUProfiler;

    /**
     * @brief 渲染系统主类
     *
     * 负责管理整个渲染管线的生命周期，包括：
     * - GPU设备和上下文的初始化
     * - 交换链和渲染通道的管理
     * - 命令缓冲区和同步对象的协调
     * - 渲染资源的创建和销毁
     * - 每帧的渲染循环
     */
    class RenderSystem
    {
    public:
        /**
         * @brief 构造函数
         */
        RenderSystem() = default;

        /**
         * @brief 析构函数
         */
        ~RenderSystem() = default;

        /**
         * @brief 初始化渲染系统
         *
         * 按顺序初始化所有GPU组件：
         * 1. GPU上下文和设备
         * 2. 交换链
         * 3. 资源管理器和状态管理器
         * 4. 命令池和同步对象
         * 5. 渲染资源
         */
        void initialize();

        /**
         * @brief 清理渲染系统
         *
         * 按相反顺序销毁所有GPU资源，确保正确的清理顺序
         */
        void clear();

        /**
         * @brief 渲染系统主循环
         *
         * 每帧调用的主要渲染函数，包括：
         * - 等待上一帧完成
         * - 获取交换链图像
         * - 记录渲染命令
         * - 提交命令缓冲区
         * - 呈现图像
         *
         * @param dt 帧时间间隔（秒）
         */
        void tick(float dt);

        // ========== 访问器方法 ==========

        /**
         * @brief 获取GPU上下文
         * @return GPU上下文智能指针
         */
        std::shared_ptr<GPUContext> getContext() const { return m_context; }

        /**
         * @brief 获取GPU设备
         * @return GPU设备智能指针
         */
        std::shared_ptr<GPUDevice> getDevice() const { return m_device; }

        /**
         * @brief 获取交换链
         * @return 交换链智能指针
         */
        std::shared_ptr<GPUSwapChain> getSwapChain() const { return m_swap_chain; }

        /**
         * @brief 获取渲染管线
         * @return 渲染管线智能指针
         */
        std::shared_ptr<GPUPipeline> getPipeline() const { return m_pipeline; }

        /**
         * @brief 获取渲染通道
         * @return 渲染通道智能指针
         */
        std::shared_ptr<GPURenderPass> getRenderPass() const { return m_render_pass; }

        /**
         * @brief 获取命令池
         * @return 命令池智能指针
         */
        std::shared_ptr<GPUCommandPool> getCommandPool() const { return m_command_pool; }

        /**
         * @brief 获取同步对象
         * @return 同步对象智能指针
         */
        std::shared_ptr<GPUSyncObject> getSyncObject() const { return m_sync_object; }

        /**
         * @brief 获取渲染资源管理器
         * @return 渲染资源管理器智能指针
         */
        std::shared_ptr<GPURenderResourceManager> getResourceManager() const { return m_resource_manager; }

        /**
         * @brief 获取渲染状态管理器
         * @return 渲染状态管理器智能指针
         */
        std::shared_ptr<GPURenderStateManager> getStateManager() const { return m_state_manager; }

        /**
         * @brief 获取当前帧索引
         * @return 当前帧索引
         */
        uint32_t getCurrentFrame() const { return m_current_frame; }

        /**
         * @brief 检查帧缓冲区是否已调整大小
         * @return 如果帧缓冲区已调整大小则返回true
         */
        bool isFramebufferResized() const { return m_framebuffer_resized; }

        /**
         * @brief 设置帧缓冲区调整大小标志
         * @param resized 帧缓冲区是否已调整大小
         */
        void setFramebufferResized(bool resized) { m_framebuffer_resized = resized; }

        /**
         * @brief 获取顶点缓冲区
         * @return 顶点缓冲区的VkBuffer句柄
         * @todo 将此功能移至资源管理器中
         */
        VkBuffer getVertexBuffer() const { return m_vertex_buffer.getBuffer(); }
        VkBuffer getIndexBuffer() const { return m_index_buffer.getBuffer(); }

    private:
        // ========== 私有方法 ==========

        /**
         * @brief 初始化渲染资源
         *
         * 创建默认的渲染通道、渲染资源和渲染管线
         */
        void initializeRenderResources();

        /**
         * @brief 创建默认渲染管线
         *
         * 使用预定义的配置创建基础三角形渲染管线
         */
        void createDefaultPipeline();

        /**
         * @brief 创建默认渲染通道
         *
         * 创建基础的颜色渲染通道
         */
        void createDefaultRenderPass();

        /**
         * @brief 创建渲染资源
         *
         * 创建顶点缓冲区等基础渲染资源
         */
        void createRenderResource();

        // ========== 渲染循环辅助方法 ==========

        /**
         * @brief 获取交换链图像
         *
         * @param in_flight_fence 飞行中围栏
         * @param image_available_semaphore 图像可用信号量
         * @param image_index 输出图像索引
         * @return VK_SUCCESS表示成功，其他值表示失败或需要重建交换链
         */
        VkResult acquireNextImage(VkFence in_flight_fence, VkSemaphore image_available_semaphore, uint32_t& image_index);

        /**
         * @brief 记录渲染命令
         *
         * @param command_buffer 命令缓冲区
         * @param image_index 图像索引
         * @return true表示成功，false表示失败
         */
        bool recordRenderCommands(VkCommandBuffer command_buffer, uint32_t image_index);

        /**
         * @brief 提交命令缓冲区
         *
         * @param command_buffer 命令缓冲区
         * @param image_available_semaphore 图像可用信号量
         * @param in_flight_fence 飞行中围栏
         * @return true表示成功，false表示失败
         */
        bool submitCommandBuffer(VkCommandBuffer command_buffer, VkSemaphore image_available_semaphore, VkFence in_flight_fence);

        /**
         * @brief 呈现图像
         *
         * @param image_index 图像索引
         * @return VK_SUCCESS表示成功，其他值表示失败或需要重建交换链
         */
        VkResult presentImage(uint32_t image_index);

        /**
         * @brief 重建交换链
         *
         * 当交换链过期或窗口大小改变时调用
         */
        void recreateSwapChain();

        // ========== 成员变量 ==========

        // 核心GPU组件
        std::shared_ptr<GPUContext>     m_context;      ///< GPU上下文
        std::shared_ptr<GPUDevice>      m_device;       ///< GPU设备
        std::shared_ptr<GPUSwapChain>   m_swap_chain;   ///< 交换链
        std::shared_ptr<GPUPipeline>    m_pipeline;     ///< 渲染管线
        std::shared_ptr<GPURenderPass>  m_render_pass;  ///< 渲染通道
        std::shared_ptr<GPUCommandPool> m_command_pool; ///< 命令池
        std::shared_ptr<GPUSyncObject>  m_sync_object;  ///< 同步对象

        // 管理工具类
        std::shared_ptr<GPURenderResourceManager> m_resource_manager; ///< 渲染资源管理器
        std::shared_ptr<GPURenderStateManager>    m_state_manager;    ///< 渲染状态管理器

        // 渲染资源
        GPUBuffer m_vertex_buffer; ///< 顶点缓冲区 @todo 移至资源管理器
        GPUBuffer m_index_buffer;  ///< 索引缓冲区 @todo 移至资源管理器

        // 状态标志
        bool     m_framebuffer_resized = false; ///< 帧缓冲区调整大小标志
        uint32_t m_current_frame       = 0;     ///< 当前帧索引
    };
} // namespace Piccolo
