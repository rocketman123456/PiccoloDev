#include "runtime/function/render/gpu_command_pool.h"
#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_render_pass.h"
#include "runtime/function/render/gpu_swap_chain.h"

#include "runtime/function/profiler/gpu_profiler.h"

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
        info.pNext            = nullptr;

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

    void GPUCommandPool::recordRenderCommands(VkCommandBuffer command_buffer, int image_index)
    {
        // 注意：命令缓冲区应该已经在外部开始记录了

        auto render_pass       = g_runtime_global_context.m_render_system->getRenderPass()->getRenderPass();
        auto swap_chain_extent = g_runtime_global_context.m_render_system->getSwapChain()->getExtent();
        auto graphics_pipeline = g_runtime_global_context.m_render_system->getPipeline()->getPipeline();
        auto framebuffer       = g_runtime_global_context.m_render_system->getPipeline()->getSwapChainFramebuffers()[image_index];
        auto extent            = g_runtime_global_context.m_render_system->getSwapChain()->getExtent();
        auto current_frame     = g_runtime_global_context.m_render_system->getCurrentFrame();

        auto profiler = g_runtime_global_context.m_gpu_profiler;

        VkRenderPassBeginInfo render_pass_info {};
        render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass        = render_pass;
        render_pass_info.framebuffer       = framebuffer;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = extent;

        VkClearValue clear_values[2];
        clear_values[0].color        = {{0.02f, 0.02f, 0.05f, 1.0f}};
        clear_values[1].depthStencil = {1.0f, 0};
        render_pass_info.clearValueCount = 2;
        render_pass_info.pClearValues    = clear_values;

        // 开始渲染通道性能分析
        profiler->beginTimestamp(command_buffer, current_frame, "Render Pass");

        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            // 开始管道绑定性能分析
            profiler->beginTimestamp(command_buffer, current_frame, "Pipeline Bind");
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
            profiler->endTimestamp(command_buffer, current_frame);

            // 开始视口设置性能分析
            profiler->beginTimestamp(command_buffer, current_frame, "Viewport Setup");
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
            profiler->endTimestamp(command_buffer, current_frame);

            VkBuffer     vertex_buffers[] = {g_runtime_global_context.m_render_system->getVertexBuffer()};
            VkDeviceSize offsets[]        = {0};
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

            VkBuffer     index_buffers[] = {g_runtime_global_context.m_render_system->getIndexBuffer()};
            VkDeviceSize index_offsets[] = {0};
            vkCmdBindIndexBuffer(command_buffer, index_buffers[0], index_offsets[0], VK_INDEX_TYPE_UINT16);

            // 绑定相机描述符集（set=0）
            if (auto pipeline = g_runtime_global_context.m_render_system->getPipeline())
            {
                VkPipelineLayout layout = pipeline->getPipelineLayout();
                VkDescriptorSet  set    = g_runtime_global_context.m_render_system->getCameraDescriptorSet();
                if (set != VK_NULL_HANDLE)
                {
                    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
                }
            }

            // 开始绘制性能分析
            profiler->beginTimestamp(command_buffer, current_frame, "Draw Call");
            // vkCmdDraw(command_buffer, 3, 1, 0, 0);
            uint32_t index_count = g_runtime_global_context.m_render_system->getIndexCount();
            if (index_count == 0) index_count = 6;
            vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
            profiler->endTimestamp(command_buffer, current_frame);

            // 记录 ImGui 绘制（需要处在 render pass 内）
            g_runtime_global_context.m_render_system->recordImGuiDrawData(command_buffer);
        }
        vkCmdEndRenderPass(command_buffer);

        // 结束渲染通道性能分析
        profiler->endTimestamp(command_buffer, current_frame);
    }
} // namespace Piccolo
