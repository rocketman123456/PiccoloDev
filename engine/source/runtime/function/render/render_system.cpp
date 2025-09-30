#include "runtime/function/render/render_system.h"

#include "runtime/function/render/gpu_context.h"
#include "runtime/function/render/gpu_device.h"

#include "runtime/core/base/macro.h"
#include <memory>

namespace Piccolo
{
    void RenderSystem::initialize()
    {
        //
        m_context = std::make_shared<GPUContext>();
        m_device = std::make_shared<GPUDevice>(m_context->getInstance());
    }
    void RenderSystem::clear()
    {
        //
        m_device.reset();
        m_context.reset();
    }
    void RenderSystem::tick(float dt)
    {
        //
    }
} // namespace Piccolo
