#pragma once

#include <volk.h>
// #include <vulkan/vulkan.h>

namespace Piccolo
{
    class GPURenderPass
    {
    public:
        GPURenderPass(VkDevice device);
        ~GPURenderPass();

        VkRenderPass getRenderPass() const { return m_render_pass; }

    private:
        void createRenderPass();

        VkDevice     m_device;
        VkRenderPass m_render_pass;
    };
} // namespace Piccolo
