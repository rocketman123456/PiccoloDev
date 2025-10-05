#pragma once

#include "runtime/function/render/utils/gpu_utils.h"

#include <volk.h>
#include <memory>
#include <string>
#include <vector>

namespace Piccolo
{
    // 渲染状态管理器
    class GPURenderStateManager
    {
    public:
        explicit GPURenderStateManager(VkDevice device);
        ~GPURenderStateManager() = default;

        // 视口管理
        void setViewport(float x, float y, float width, float height, float min_depth = 0.0f, float max_depth = 1.0f);
        void setViewports(const std::vector<VkViewport>& viewports);
        const std::vector<VkViewport>& getViewports() const { return m_viewports; }

        // 裁剪管理
        void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);
        void setScissors(const std::vector<VkRect2D>& scissors);
        const std::vector<VkRect2D>& getScissors() const { return m_scissors; }

        // 清除值管理
        void setClearColor(float r, float g, float b, float a);
        void setClearColor(const VkClearColorValue& color);
        const VkClearColorValue& getClearColor() const { return m_clear_color; }

        void setClearDepthStencil(float depth, uint32_t stencil);
        void setClearDepthStencil(const VkClearDepthStencilValue& depth_stencil);
        const VkClearDepthStencilValue& getClearDepthStencil() const { return m_clear_depth_stencil; }

        // 渲染区域管理
        void setRenderArea(int32_t x, int32_t y, uint32_t width, uint32_t height);
        void setRenderArea(const VkRect2D& area);
        const VkRect2D& getRenderArea() const { return m_render_area; }

        // 状态应用
        void applyViewportState(VkCommandBuffer command_buffer) const;
        void applyScissorState(VkCommandBuffer command_buffer) const;
        void applyClearValues(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer) const;

        // 重置状态
        void reset();
        void resetViewports();
        void resetScissors();
        void resetClearValues();
        void resetRenderArea();

        // 获取状态信息
        bool hasViewports() const { return !m_viewports.empty(); }
        bool hasScissors() const { return !m_scissors.empty(); }
        bool hasRenderArea() const { return m_render_area.extent.width > 0 && m_render_area.extent.height > 0; }

    private:
        VkDevice m_device;
        std::vector<VkViewport> m_viewports;
        std::vector<VkRect2D> m_scissors;
        VkClearColorValue m_clear_color;
        VkClearDepthStencilValue m_clear_depth_stencil;
        VkRect2D m_render_area;
    };
} // namespace Piccolo
