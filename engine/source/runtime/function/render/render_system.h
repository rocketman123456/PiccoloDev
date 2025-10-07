#pragma once

#include <memory>

namespace Piccolo
{
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

    class RenderSystem
    {
    public:
        RenderSystem()  = default;
        ~RenderSystem() = default;

        void initialize();
        void clear();
        void tick(float dt);

        std::shared_ptr<GPUContext>     getContext() const { return m_context; }
        std::shared_ptr<GPUDevice>      getDevice() const { return m_device; }
        std::shared_ptr<GPUSwapChain>   getSwapChain() const { return m_swap_chain; }
        std::shared_ptr<GPUPipeline>    getPipeline() const { return m_pipeline; }
        std::shared_ptr<GPURenderPass>  getRenderPass() const { return m_render_pass; }
        std::shared_ptr<GPUCommandPool> getCommandPool() const { return m_command_pool; }
        std::shared_ptr<GPUSyncObject>  getSyncObject() const { return m_sync_object; }

        // 新的工具类访问器
        std::shared_ptr<GPURenderResourceManager> getResourceManager() const { return m_resource_manager; }
        std::shared_ptr<GPURenderStateManager>    getStateManager() const { return m_state_manager; }

        uint32_t getCurrentFrame() const { return m_current_frame; }

    private:
        void initializeRenderResources();
        void createDefaultPipeline();
        void createDefaultRenderPass();

        std::shared_ptr<GPUContext>     m_context;
        std::shared_ptr<GPUDevice>      m_device;
        std::shared_ptr<GPUSwapChain>   m_swap_chain;
        std::shared_ptr<GPUPipeline>    m_pipeline;
        std::shared_ptr<GPURenderPass>  m_render_pass;
        std::shared_ptr<GPUCommandPool> m_command_pool;
        std::shared_ptr<GPUSyncObject>  m_sync_object;

        // 新的工具类
        std::shared_ptr<GPURenderResourceManager> m_resource_manager;
        std::shared_ptr<GPURenderStateManager>    m_state_manager;

        uint32_t m_current_frame = 0; // 当前帧索引
    };
} // namespace Piccolo
