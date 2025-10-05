#include "runtime/function/render/gpu_render_state_manager.h"

#include "runtime/core/log/log_system.h"
#include "runtime/core/base/macro.h"

#include <algorithm>

namespace Piccolo
{
    GPURenderStateManager::GPURenderStateManager(VkDevice device) : m_device(device)
    {
        // 初始化默认值
        m_clear_color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        m_clear_depth_stencil = {1.0f, 0};
        m_render_area = {{0, 0}, {0, 0}};
        
        LOG_INFO("GPURenderStateManager initialized");
    }

    void GPURenderStateManager::setViewport(float x, float y, float width, float height, float min_depth, float max_depth)
    {
        VkViewport viewport{};
        viewport.x = x;
        viewport.y = y;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = min_depth;
        viewport.maxDepth = max_depth;

        m_viewports.clear();
        m_viewports.push_back(viewport);
    }

    void GPURenderStateManager::setViewports(const std::vector<VkViewport>& viewports)
    {
        m_viewports = viewports;
    }

    void GPURenderStateManager::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        VkRect2D scissor{};
        scissor.offset = {x, y};
        scissor.extent = {width, height};

        m_scissors.clear();
        m_scissors.push_back(scissor);
    }

    void GPURenderStateManager::setScissors(const std::vector<VkRect2D>& scissors)
    {
        m_scissors = scissors;
    }

    void GPURenderStateManager::setClearColor(float r, float g, float b, float a)
    {
        m_clear_color.float32[0] = r;
        m_clear_color.float32[1] = g;
        m_clear_color.float32[2] = b;
        m_clear_color.float32[3] = a;
    }

    void GPURenderStateManager::setClearColor(const VkClearColorValue& color)
    {
        m_clear_color = color;
    }

    void GPURenderStateManager::setClearDepthStencil(float depth, uint32_t stencil)
    {
        m_clear_depth_stencil.depth = depth;
        m_clear_depth_stencil.stencil = stencil;
    }

    void GPURenderStateManager::setClearDepthStencil(const VkClearDepthStencilValue& depth_stencil)
    {
        m_clear_depth_stencil = depth_stencil;
    }

    void GPURenderStateManager::setRenderArea(int32_t x, int32_t y, uint32_t width, uint32_t height)
    {
        m_render_area.offset = {x, y};
        m_render_area.extent = {width, height};
    }

    void GPURenderStateManager::setRenderArea(const VkRect2D& area)
    {
        m_render_area = area;
    }

    void GPURenderStateManager::applyViewportState(VkCommandBuffer command_buffer) const
    {
        if (!m_viewports.empty())
        {
            vkCmdSetViewport(command_buffer, 0, static_cast<uint32_t>(m_viewports.size()), m_viewports.data());
        }
    }

    void GPURenderStateManager::applyScissorState(VkCommandBuffer command_buffer) const
    {
        if (!m_scissors.empty())
        {
            vkCmdSetScissor(command_buffer, 0, static_cast<uint32_t>(m_scissors.size()), m_scissors.data());
        }
    }

    void GPURenderStateManager::applyClearValues(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer) const
    {
        // 这里可以根据需要实现更复杂的清除值应用逻辑
        // 目前只是提供接口，具体实现可以根据渲染通道的附件类型来决定
        LOG_DEBUG("Applying clear values to command buffer");
    }

    void GPURenderStateManager::reset()
    {
        resetViewports();
        resetScissors();
        resetClearValues();
        resetRenderArea();
    }

    void GPURenderStateManager::resetViewports()
    {
        m_viewports.clear();
    }

    void GPURenderStateManager::resetScissors()
    {
        m_scissors.clear();
    }

    void GPURenderStateManager::resetClearValues()
    {
        m_clear_color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        m_clear_depth_stencil = {1.0f, 0};
    }

    void GPURenderStateManager::resetRenderArea()
    {
        m_render_area = {{0, 0}, {0, 0}};
    }
} // namespace Piccolo
