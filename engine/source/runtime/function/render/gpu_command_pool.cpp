#include "runtime/function/render/gpu_command_pool.h"
#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_render_pass.h"
#include "runtime/function/render/gpu_swap_chain.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

namespace Piccolo
{
    GPUCommandPool::GPUCommandPool(VkDevice device, uint32_t max_frames_in_flight)
    {
        m_device               = device;
        m_max_frames_in_flight = max_frames_in_flight;

        createCommandPool();
        createCommandBuffers();
    }

    GPUCommandPool::~GPUCommandPool()
    {
        // not need to destroy command buffers
        vkDestroyCommandPool(m_device, m_command_pool, nullptr);
    }

    void GPUCommandPool::createCommandPool()
    {
        auto queue_family_index = g_runtime_global_context.m_render_system->getDevice()->getGraphicsQueueFamily();

        VkCommandPoolCreateInfo info;
        info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex = queue_family_index;

        if (vkCreateCommandPool(m_device, &info, nullptr, &m_command_pool) != VK_SUCCESS)
        {
            LOG_ERROR("failed to create command pool!");
        }
    }

    void GPUCommandPool::createCommandBuffers()
    {
        m_command_buffers.resize(m_max_frames_in_flight);

        VkCommandBufferAllocateInfo alloc_info {};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool        = m_command_pool;
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = m_max_frames_in_flight;

        if (vkAllocateCommandBuffers(m_device, &alloc_info, m_command_buffers.data()) != VK_SUCCESS)
        {
            LOG_ERROR("failed to allocate command buffers!");
        }
    }

    void GPUCommandPool::recordCommandBuffer(VkCommandBuffer command_buffer, int image_index)
    {
        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        {
            LOG_ERROR("failed to begin recording command buffer!");
            return;
        }

        auto render_pass       = g_runtime_global_context.m_render_system->getRenderPass()->getRenderPass();
        auto swap_chain_extent = g_runtime_global_context.m_render_system->getSwapChain()->getExtent();
        auto graphics_pipeline = g_runtime_global_context.m_render_system->getPipeline()->getPipeline();
        auto framebuffer       = g_runtime_global_context.m_render_system->getPipeline()->getSwapChainFramebuffers()[image_index];
        auto extent            = g_runtime_global_context.m_render_system->getSwapChain()->getExtent();

        VkRenderPassBeginInfo render_pass_info {};
        render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass        = render_pass;
        render_pass_info.framebuffer       = framebuffer;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = extent;

        VkClearValue clear_color         = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues    = &clear_color;

        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

            VkViewport viewport {};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = static_cast<float>(extent.width);
            viewport.height   = static_cast<float>(extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(command_buffer, 0, 1, &viewport);

            VkRect2D scissor {};
            scissor.offset = {0, 0};
            scissor.extent = extent;
            vkCmdSetScissor(command_buffer, 0, 1, &scissor);

            vkCmdDraw(command_buffer, 3, 1, 0, 0);
        }
        vkCmdEndRenderPass(command_buffer);

        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        {
            LOG_ERROR("failed to record command buffer!");
        }
    }
} // namespace Piccolo
