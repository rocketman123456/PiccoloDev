#include "runtime/function/render/render_system.h"

#include "runtime/function/render/gpu_command_pool.h"
#include "runtime/function/render/gpu_context.h"
#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_pipeline_manager.h"
#include "runtime/function/render/gpu_render_pass.h"
#include "runtime/function/render/gpu_render_resource_manager.h"
#include "runtime/function/render/gpu_render_state_manager.h"
#include "runtime/function/render/gpu_swap_chain.h"
#include "runtime/function/render/gpu_sync_object.h"
#include "runtime/function/render/profiler/cpu_profiler.h"
#include "runtime/function/render/profiler/gpu_profiler.h"

#include "runtime/core/base/macro.h"

#include <memory>

namespace Piccolo
{
    void RenderSystem::initialize()
    {
        // 初始化基础GPU组件
        m_context    = std::make_shared<GPUContext>();
        m_device     = std::make_shared<GPUDevice>(m_context->getInstance());
        m_swap_chain = std::make_shared<GPUSwapChain>(m_device->getPhysicalDevice(), m_device->getDevice(), m_device->getSurface());

        // 初始化新的工具类
        m_resource_manager = std::make_shared<GPURenderResourceManager>(m_device->getDevice());
        m_state_manager    = std::make_shared<GPURenderStateManager>(m_device->getDevice());

        // 初始化性能分析器
        m_gpu_profiler = std::make_shared<GPUProfiler>();
        m_gpu_profiler->initialize(m_device->getDevice(), m_device->getPhysicalDevice(), 64, 3); // 每帧64个查询，最多3帧

        m_cpu_profiler = std::make_shared<CPUProfiler>();
        m_cpu_profiler->initialize();

        // 初始化渲染资源
        initializeRenderResources();

        // 使用交换链图像数量作为最大并发帧数，确保每个图像都有独立的命令缓冲区和同步对象
        uint32_t max_frames_in_flight = static_cast<uint32_t>(m_swap_chain->getImages().size());

        m_command_pool = std::make_shared<GPUCommandPool>(m_device->getDevice(), max_frames_in_flight);
        m_sync_object  = std::make_shared<GPUSyncObject>(m_device->getDevice(), max_frames_in_flight);
    }

    void RenderSystem::clear()
    {
        vkDeviceWaitIdle(m_device->getDevice());

        // 清理性能分析器
        m_gpu_profiler.reset();
        m_cpu_profiler.reset();

        // 清理新的工具类
        m_state_manager.reset();
        m_resource_manager.reset();

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
        // 开始新帧的性能分析
        m_gpu_profiler->beginFrame(m_current_frame);
        m_cpu_profiler->beginFrame(m_current_frame);

        auto in_flight_fence           = m_sync_object->getInFlightFence(m_current_frame);
        auto image_available_semaphore = m_sync_object->getImageAvailableSemaphore(m_current_frame);
        auto command_buffer            = m_command_pool->getCommandBuffer(m_current_frame);

        // draw frame
        m_cpu_profiler->beginTimestamp("Frame Wait");
        vkWaitForFences(m_device->getDevice(), 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device->getDevice(), 1, &in_flight_fence);
        uint32_t image_index;
        vkAcquireNextImageKHR(m_device->getDevice(), m_swap_chain->getSwapchain(), UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
        m_cpu_profiler->endTimestamp("Frame Wait");

        m_cpu_profiler->beginTimestamp("Command Buffer Reset");
        vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        m_cpu_profiler->endTimestamp("Command Buffer Reset");

        // 开始记录命令缓冲区
        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        m_cpu_profiler->beginTimestamp("Command Buffer Begin");
        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        {
            LOG_ERROR("failed to begin recording command buffer!");
            return;
        }
        m_cpu_profiler->endTimestamp("Command Buffer Begin");

        m_gpu_profiler->resetQueryPool(command_buffer, m_current_frame);

        // 开始性能分析 - 必须在命令缓冲区开始记录后调用
        m_gpu_profiler->beginTimestamp(command_buffer, m_current_frame, "Command Record");
        m_cpu_profiler->beginTimestamp("Command Record");
        // 记录渲染命令
        m_command_pool->recordRenderCommands(command_buffer, image_index);
        // 结束性能分析
        m_cpu_profiler->endTimestamp("Command Record");
        m_gpu_profiler->endTimestamp(command_buffer, m_current_frame);

        m_cpu_profiler->beginTimestamp("Command Buffer End");
        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        {
            LOG_ERROR("failed to record command buffer!");
            return;
        }
        m_cpu_profiler->endTimestamp("Command Buffer End");

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore          wait_semaphores[] = {image_available_semaphore};
        VkPipelineStageFlags wait_stages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount         = 1;
        submit_info.pWaitSemaphores            = wait_semaphores;
        submit_info.pWaitDstStageMask          = wait_stages;

        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffer;

        VkSemaphore signal_semaphores[]  = {m_sync_object->getRenderFinishedSemaphore(m_current_frame)};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        m_cpu_profiler->beginTimestamp("Queue Submit");
        if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submit_info, m_sync_object->getInFlightFence(m_current_frame)) != VK_SUCCESS)
        {
            LOG_ERROR("failed to submit draw command buffer!");
            return; // 提交失败时直接返回，避免继续执行可能导致的问题
        }
        m_cpu_profiler->endTimestamp("Queue Submit");

        VkPresentInfoKHR present_info {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {m_swap_chain->getSwapchain()};
        present_info.swapchainCount  = 1;
        present_info.pSwapchains     = swap_chains;

        present_info.pImageIndices = &image_index;

        m_cpu_profiler->beginTimestamp("Present");
        vkQueuePresentKHR(m_device->getPresentQueue(), &present_info);
        m_cpu_profiler->endTimestamp("Present");

        // 结束帧性能分析并记录数据
        m_gpu_profiler->endFrame(m_current_frame);
        m_cpu_profiler->endFrame(m_current_frame);

        // 每帧输出性能数据（用于调试）
        static uint32_t frame_count = 0;
        frame_count++;

        // 简单显示帧计数
        LOG_INFO("=== Frame {} completed ===", frame_count);

        if (m_gpu_profiler->hasValidData() || m_cpu_profiler->hasValidData())
        {
            logProfilerData();
        }
        else
        {
            LOG_INFO("No profiler data available for frame {}", frame_count);
        }

        // 更新帧索引，循环使用同步对象
        m_current_frame = (m_current_frame + 1) % m_sync_object->getMaxFramesInFlight();
    }

    void RenderSystem::initializeRenderResources()
    {
        // 创建默认渲染通道
        createDefaultRenderPass();

        // 创建默认管道
        createDefaultPipeline();
    }

    void RenderSystem::createDefaultRenderPass()
    {
        // 使用新的工厂方法创建基础颜色渲染通道
        auto color_format = m_swap_chain->getImageFormat();
        m_render_pass     = GPURenderPass::createBasicColorPass(m_device->getDevice(), color_format);
    }

    void RenderSystem::createDefaultPipeline()
    {
        // 使用新的工厂方法创建基础三角形管道
        m_pipeline = GPUPipeline::createBasicTrianglePipeline(m_device->getDevice(), m_render_pass->getRenderPass());
    }

    void RenderSystem::logProfilerData()
    {
        bool has_gpu_data = m_gpu_profiler->hasValidData();
        bool has_cpu_data = m_cpu_profiler->hasValidData();

        if (!has_gpu_data && !has_cpu_data)
        {
            LOG_INFO("=== Profiler Data (Frame {}) - No valid data ===", m_current_frame);
            return;
        }

        LOG_INFO("=== Profiler Data (Frame {}) ===", m_current_frame);

        // 输出GPU profiler数据
        if (has_gpu_data)
        {
            const GPUTimestamp* gpu_timestamps = m_gpu_profiler->getTimestamps();
            uint32_t            gpu_count      = m_gpu_profiler->getTimestampCount();

            LOG_INFO("--- GPU Profiler Data ({} timestamps) ---", gpu_count);
            for (uint32_t i = 0; i < gpu_count; ++i)
            {
                const GPUTimestamp& timestamp = gpu_timestamps[i];
                LOG_INFO(
                    "  GPU {}: {:.3f} ms (start: {}, end: {})",
                    timestamp.name ? timestamp.name : "Unknown",
                    timestamp.elapsed_ms,
                    timestamp.start,
                    timestamp.end
                );
            }
        }
        else
        {
            LOG_INFO("--- GPU Profiler Data - No valid data ---");
        }

        // 输出CPU profiler数据
        if (has_cpu_data)
        {
            const CPUTimestamp* cpu_timestamps = m_cpu_profiler->getTimestamps();
            uint32_t            cpu_count      = m_cpu_profiler->getTimestampCount();

            LOG_INFO("--- CPU Profiler Data ({} timestamps) ---", cpu_count);
            for (uint32_t i = 0; i < cpu_count; ++i)
            {
                const CPUTimestamp& timestamp = cpu_timestamps[i];
                // LOG_INFO("  CPU {}: {:.3f} ms (start: {:.3f}, end: {:.3f})", timestamp.name, timestamp.elapsed_ms, timestamp.start_time, timestamp.end_time);
                LOG_INFO("  CPU {}: {:.3f} ms", timestamp.name, timestamp.elapsed_ms);
            }
        }
        else
        {
            LOG_INFO("--- CPU Profiler Data - No valid data ---");
        }

        LOG_INFO("=== End Profiler Data ===");
    }
} // namespace Piccolo
