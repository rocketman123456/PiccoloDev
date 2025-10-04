#pragma once

#include <memory>

namespace Piccolo
{
    class GPUContext;
    class GPUDevice;
    class GPUSwapChain;
    class GPUPipeline;
    class GPURenderPass;

    class RenderSystem
    {
    public:
        RenderSystem()  = default;
        ~RenderSystem() = default;

        void initialize();
        void clear();
        void tick(float dt);

        std::shared_ptr<GPUContext>    getContext() const { return m_context; }
        std::shared_ptr<GPUDevice>     getDevice() const { return m_device; }
        std::shared_ptr<GPUSwapChain>  getSwapChain() const { return m_swap_chain; }
        std::shared_ptr<GPUPipeline>   getPipeline() const { return m_pipeline; }
        std::shared_ptr<GPURenderPass> getRenderPass() const { return m_render_pass; }

    private:
        std::shared_ptr<GPUContext>    m_context;
        std::shared_ptr<GPUDevice>     m_device;
        std::shared_ptr<GPUSwapChain>  m_swap_chain;
        std::shared_ptr<GPUPipeline>   m_pipeline;
        std::shared_ptr<GPURenderPass> m_render_pass;
    };
} // namespace Piccolo
