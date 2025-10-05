#include "runtime/function/render/render_pipeline_base.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/debugdraw/debug_draw_manager.h"

namespace Piccolo
{
    bool RenderPipelineBase::preparePassData(std::shared_ptr<RenderResourceBase> render_resource)
    {
        m_main_camera_pass->preparePassData(render_resource);
        m_pick_pass->preparePassData(render_resource);
        m_directional_light_pass->preparePassData(render_resource);
        m_point_light_shadow_pass->preparePassData(render_resource);
        m_particle_pass->preparePassData(render_resource);
        g_runtime_global_context.m_debugdraw_manager->preparePassData(render_resource);
        return true;
    }
    
    bool RenderPipelineBase::forwardRender(std::shared_ptr<RHI> /* rhi */, std::shared_ptr<RenderResourceBase> /* render_resource */) 
    {
        // 基类默认实现，子类应该重写
        return false;
    }
    
    bool RenderPipelineBase::deferredRender(std::shared_ptr<RHI> /* rhi */, std::shared_ptr<RenderResourceBase> /* render_resource */) 
    {
        // 基类默认实现，子类应该重写
        return false;
    }
    
    bool RenderPipelineBase::initializeUIRenderBackend(WindowUI* window_ui) 
    { 
        if (m_ui_pass) {
            m_ui_pass->initializeUIRenderBackend(window_ui);
            return true;
        }
        return false;
    }

    void RenderPipelineBase::clear()
    {
        // 清理所有渲染通道
        cleanupRenderPasses();
        
        // 重置状态
        m_state = RenderPipelineState::UNINITIALIZED;
        m_last_error.clear();
    }

    void RenderPipelineBase::cleanupRenderPasses()
    {
        // 清理所有渲染通道资源
        m_main_camera_pass.reset();
        m_pick_pass.reset();
        m_directional_light_pass.reset();
        m_point_light_shadow_pass.reset();
        m_particle_pass.reset();
        m_tone_mapping_pass.reset();
        m_color_grading_pass.reset();
        m_vignette_pass.reset();
        m_ui_pass.reset();
        m_combine_ui_pass.reset();
        m_fxaa_pass.reset();
    }
} // namespace Piccolo
