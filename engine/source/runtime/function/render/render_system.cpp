#include "runtime/function/render/render_system.h"

#include "runtime/function/render/gpu_context.h"
#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_pipeline_manager.h"
#include "runtime/function/render/gpu_swap_chain.h"

#include "runtime/core/base/macro.h"

#include <memory>

namespace Piccolo
{
    void RenderSystem::initialize()
    {
        m_context    = std::make_shared<GPUContext>();
        m_device     = std::make_shared<GPUDevice>(m_context->getInstance());
        m_swap_chain = std::make_shared<GPUSwapChain>(m_device->getPhysicalDevice(), m_device->getDevice(), m_device->getSurface());

        // PipelineConfig pipeline_config;
        // pipeline_config.shader_paths = {
        //     "asset/shader/glsl/_shader_base.vert",
        //     "asset/shader/glsl/_shader_base.frag",
        // };
        // pipeline_config.types = {
        //     ShaderType::VertexShader,
        //     ShaderType::FragmanetShader,
        // };

        GPUPipelineConfig pipeline_config;
        pipeline_config.name          = "DefaultRenderPipeline";
        pipeline_config.description   = "Default render pipeline with all standard passes";
        pipeline_config.shader_stages = {
            {   ShaderType::VertexShader, "asset/shader/glsl/_shader_base.vert", "main"},
            {ShaderType::FragmanetShader, "asset/shader/glsl/_shader_base.frag", "main"},
        };

        m_pipeline = std::make_shared<GPUPipeline>(m_device->getDevice(), pipeline_config);
    }

    void RenderSystem::clear()
    {
        m_pipeline.reset();
        m_swap_chain.reset();
        m_device.reset();
        m_context.reset();
    }
    void RenderSystem::tick(float dt)
    {
        //
    }
} // namespace Piccolo
