#include "runtime/function/render/render_system.h"

#include "runtime/function/render/gpu_command_pool.h"
#include "runtime/function/render/gpu_context.h"
#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_pipeline_manager.h"
#include "runtime/function/render/gpu_render_pass.h"
#include "runtime/function/render/gpu_swap_chain.h"
#include "runtime/function/render/gpu_sync_object.h"

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

        m_render_pass = std::make_shared<GPURenderPass>(m_device->getDevice());

        GPUPipelineConfig pipeline_config;
        pipeline_config.name          = "DefaultRenderPipeline";
        pipeline_config.description   = "Default render pipeline with all standard passes";
        pipeline_config.shader_stages = {
            {   ShaderType::VertexShader, "asset/shader/glsl/_shader_base.vert", "main"},
            {ShaderType::FragmanetShader, "asset/shader/glsl/_shader_base.frag", "main"},
        };

        m_pipeline = std::make_shared<GPUPipeline>(m_device->getDevice(), pipeline_config);

        // 使用交换链图像数量作为最大并发帧数，确保每个图像都有独立的命令缓冲区和同步对象
        uint32_t max_frames_in_flight = static_cast<uint32_t>(m_swap_chain->getImages().size());
        m_command_pool                = std::make_shared<GPUCommandPool>(m_device->getDevice(), max_frames_in_flight);
        m_sync_object                 = std::make_shared<GPUSyncObject>(m_device->getDevice(), max_frames_in_flight);
    }

    void RenderSystem::clear()
    {
        vkDeviceWaitIdle(m_device->getDevice());

        m_sync_object.reset();
        m_command_pool.reset();
        m_pipeline.reset();
        m_render_pass.reset();
        m_swap_chain.reset();
        m_device.reset();
        m_context.reset();
    }
    void RenderSystem::tick(float /*dt*/)
    {
        auto in_flight_fence = m_sync_object->getInFlightFence(m_current_frame);

        // draw frame
        vkWaitForFences(m_device->getDevice(), 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device->getDevice(), 1, &in_flight_fence);

        uint32_t image_index;
        vkAcquireNextImageKHR(
            m_device->getDevice(),
            m_swap_chain->getSwapchain(),
            UINT64_MAX,
            m_sync_object->getImageAvailableSemaphore(m_current_frame),
            VK_NULL_HANDLE,
            &image_index
        );

        auto command_buffer = m_command_pool->getCommandBuffer(m_current_frame);

        vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        m_command_pool->recordCommandBuffer(command_buffer, image_index);

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore          wait_semaphores[] = {m_sync_object->getImageAvailableSemaphore(m_current_frame)};
        VkPipelineStageFlags wait_stages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount         = 1;
        submit_info.pWaitSemaphores            = wait_semaphores;
        submit_info.pWaitDstStageMask          = wait_stages;

        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffer;

        VkSemaphore signal_semaphores[]  = {m_sync_object->getRenderFinishedSemaphore(m_current_frame)};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submit_info, m_sync_object->getInFlightFence(m_current_frame)) != VK_SUCCESS)
        {
            LOG_ERROR("failed to submit draw command buffer!");
            return; // 提交失败时直接返回，避免继续执行可能导致的问题
        }

        VkPresentInfoKHR present_info {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {m_swap_chain->getSwapchain()};
        present_info.swapchainCount  = 1;
        present_info.pSwapchains     = swap_chains;

        present_info.pImageIndices = &image_index;

        vkQueuePresentKHR(m_device->getPresentQueue(), &present_info);

        // 更新帧索引，循环使用同步对象
        m_current_frame = (m_current_frame + 1) % m_sync_object->getMaxFramesInFlight();
    }
} // namespace Piccolo
