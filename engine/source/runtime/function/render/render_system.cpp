#include "runtime/function/render/render_system.h"

#include "runtime/function/render/gpu_context.h"
#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/gpu_swap_chain.h"

#include "runtime/core/base/macro.h"

#include <memory>

namespace Piccolo
{
    void RenderSystem::initialize()
    {
        m_context    = std::make_shared<GPUContext>();
        m_device     = std::make_shared<GPUDevice>(m_context->getInstance());
        // m_swap_chain = std::make_shared<GPUSwapChain>(m_device->getPhysicalDevice(), m_device->getDevice(), m_device->getSurface());
    }
    void RenderSystem::clear()
    {
        // m_swap_chain.reset();
        m_device.reset();
        m_context.reset();
    }
    void RenderSystem::tick(float dt)
    {
        //
    }
} // namespace Piccolo
