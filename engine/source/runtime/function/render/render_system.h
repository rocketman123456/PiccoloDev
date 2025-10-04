#pragma once

#include <memory>

namespace Piccolo
{
    class GPUContext;
    class GPUDevice;
    class GPUSwapChain;
    class GPUPipeline;

    class RenderSystem
    {
    public:
        RenderSystem()  = default;
        ~RenderSystem() = default;

        void initialize();
        void clear();
        void tick(float dt);

    private:
        std::shared_ptr<GPUContext>   m_context;
        std::shared_ptr<GPUDevice>    m_device;
        std::shared_ptr<GPUSwapChain> m_swap_chain;
        std::shared_ptr<GPUPipeline>  m_pipeline;
    };
} // namespace Piccolo
