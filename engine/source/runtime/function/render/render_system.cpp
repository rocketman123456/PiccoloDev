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
#include "runtime/function/render/utils/gpu_buffer_utils.h"
#include "runtime/function/render/utils/gpu_pipeline_builder.h"
#include "runtime/function/render/utils/gpu_render_pass_builder.h"

#include "runtime/function/profiler/cpu_profiler.h"
#include "runtime/function/profiler/gpu_profiler.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/gpu_render_resource.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include <array>

namespace Piccolo
{
    // const std::vector<Vertex> vertices = {
    //     {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    //     { {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    //     {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    // };

    const std::vector<Vertex> vertices = {
        {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        { {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    void RenderSystem::initialize()
    {
        // 初始化基础GPU组件
        m_context    = std::make_shared<GPUContext>();
        m_device     = std::make_shared<GPUDevice>(m_context->getInstance());
        m_swap_chain = std::make_shared<GPUSwapChain>(m_device->getPhysicalDevice(), m_device->getDevice(), m_device->getSurface());

        // 初始化新的工具类
        m_resource_manager = std::make_shared<GPURenderResourceManager>(m_device->getDevice(), m_device->getPhysicalDevice());
        m_state_manager    = std::make_shared<GPURenderStateManager>(m_device->getDevice());

        // 使用交换链图像数量作为最大并发帧数，确保每个图像都有独立的命令缓冲区和同步对象
        uint32_t max_frames_in_flight = static_cast<uint32_t>(m_swap_chain->getImages().size());

        m_command_pool = std::make_shared<GPUCommandPool>(m_device->getDevice(), max_frames_in_flight);
        m_sync_object  = std::make_shared<GPUSyncObject>(m_device->getDevice(), max_frames_in_flight);

        // 初始化渲染资源
        initializeRenderResources();
    }

    void RenderSystem::clear()
    {
        vkDeviceWaitIdle(m_device->getDevice());

        vkDestroyBuffer(m_device->getDevice(), m_vertex_buffer, nullptr);
        vkFreeMemory(m_device->getDevice(), m_vertex_buffer_memory, nullptr);

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
        auto gpu_profiler = g_runtime_global_context.m_gpu_profiler;
        auto cpu_profiler = g_runtime_global_context.m_cpu_profiler;

        // gpu_profiler->beginFrame(m_current_frame);
        cpu_profiler->beginFrame(m_current_frame);

        auto in_flight_fence           = m_sync_object->getInFlightFence(m_current_frame);
        auto image_available_semaphore = m_sync_object->getImageAvailableSemaphore(m_current_frame);
        auto command_buffer            = m_command_pool->getCommandBuffer(m_current_frame);

        // draw frame
        cpu_profiler->beginTimestamp("Frame Wait");
        vkWaitForFences(m_device->getDevice(), 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device->getDevice(), 1, &in_flight_fence);
        uint32_t image_index;
        VkResult result =
            vkAcquireNextImageKHR(m_device->getDevice(), m_swap_chain->getSwapchain(), UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
        cpu_profiler->endTimestamp("Frame Wait");

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            cpu_profiler->beginTimestamp("Recreate Swap Chain");
            m_swap_chain->recreateSwapChain();
            m_pipeline->destroyFramebuffers();
            m_pipeline->createFramebuffers();
            cpu_profiler->endTimestamp("Recreate Swap Chain");
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            LOG_ERROR("failed to acquire swap chain image!");
            return;
        }

        cpu_profiler->beginTimestamp("Command Buffer Reset");
        vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        cpu_profiler->endTimestamp("Command Buffer Reset");

        // 开始记录命令缓冲区
        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        cpu_profiler->beginTimestamp("Command Buffer Begin");
        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        {
            LOG_ERROR("failed to begin recording command buffer!");
            return;
        }
        cpu_profiler->endTimestamp("Command Buffer Begin");

        // 临时禁用GPU profiler来测试
        // gpu_profiler->resetQueryPool(command_buffer, m_current_frame);

        // 开始性能分析 - 必须在命令缓冲区开始记录后调用
        // gpu_profiler->beginTimestamp(command_buffer, m_current_frame, "Command Record");
        cpu_profiler->beginTimestamp("Command Record");
        // 记录渲染命令
        m_command_pool->recordRenderCommands(command_buffer, image_index);
        // 结束性能分析
        cpu_profiler->endTimestamp("Command Record");
        // gpu_profiler->endTimestamp(command_buffer, m_current_frame);

        cpu_profiler->beginTimestamp("Command Buffer End");
        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        {
            LOG_ERROR("failed to record command buffer!");
            return;
        }
        cpu_profiler->endTimestamp("Command Buffer End");

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

        cpu_profiler->beginTimestamp("Queue Submit");
        if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submit_info, m_sync_object->getInFlightFence(m_current_frame)) != VK_SUCCESS)
        {
            LOG_ERROR("failed to submit draw command buffer!");
            return; // 提交失败时直接返回，避免继续执行可能导致的问题
        }
        cpu_profiler->endTimestamp("Queue Submit");

        VkPresentInfoKHR present_info {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = signal_semaphores;

        VkSwapchainKHR swap_chains[] = {m_swap_chain->getSwapchain()};
        present_info.swapchainCount  = 1;
        present_info.pSwapchains     = swap_chains;

        present_info.pImageIndices = &image_index;

        cpu_profiler->beginTimestamp("Present");
        result = vkQueuePresentKHR(m_device->getPresentQueue(), &present_info);
        cpu_profiler->endTimestamp("Present");

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
        {
            m_framebuffer_resized = false;

            cpu_profiler->beginTimestamp("Recreate Swap Chain");
            m_swap_chain->recreateSwapChain();
            m_pipeline->destroyFramebuffers();
            m_pipeline->createFramebuffers();
            cpu_profiler->endTimestamp("Recreate Swap Chain");
        }
        else if (result != VK_SUCCESS)
        {
            LOG_ERROR("failed to present swap chain image!");
            return;
        }

        // 结束帧性能分析并记录数据
        // gpu_profiler->endFrame(m_current_frame);
        cpu_profiler->endFrame(m_current_frame);

        // 更新帧索引，循环使用同步对象
        m_current_frame = (m_current_frame + 1) % m_sync_object->getMaxFramesInFlight();
    }

    void RenderSystem::initializeRenderResources()
    {
        // 创建默认渲染通道
        createDefaultRenderPass();

        // 创建渲染资源
        createRenderResource();

        // 创建默认管道
        createDefaultPipeline();
    }

    void RenderSystem::createDefaultRenderPass()
    {
        // 使用新的工厂方法创建基础颜色渲染通道
        auto color_format = m_swap_chain->getImageFormat();

        auto config   = GPURenderPassConfigFactory::createBasicColorPass(color_format);
        m_render_pass = std::make_shared<GPURenderPass>(m_device->getDevice(), config);
    }

    void RenderSystem::createDefaultPipeline()
    {
        auto config = GPUPipelineConfigFactory::createAdvancedTrianglePipeline();
        m_pipeline  = std::make_shared<GPUPipeline>(m_device->getDevice(), config, m_render_pass->getRenderPass());
    }

    void RenderSystem::createRenderResource()
    {
        // VkBufferCreateInfo buffer_info {};
        // buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        // buffer_info.size        = sizeof(vertices[0]) * vertices.size();
        // buffer_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        // buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // if (vkCreateBuffer(m_device->getDevice(), &buffer_info, nullptr, &m_vertex_buffer) != VK_SUCCESS)
        // {
        //     LOG_ERROR("failed to create vertex buffer!");
        //     return;
        // }

        // VkMemoryRequirements mem_requirements;
        // vkGetBufferMemoryRequirements(m_device->getDevice(), m_vertex_buffer, &mem_requirements);

        // VkMemoryAllocateInfo alloc_info {};
        // alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        // alloc_info.allocationSize  = mem_requirements.size;
        // alloc_info.memoryTypeIndex = find_memory_type(
        //     m_device->getPhysicalDevice(), mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        // );

        // if (vkAllocateMemory(m_device->getDevice(), &alloc_info, nullptr, &m_vertex_buffer_memory) != VK_SUCCESS)
        // {
        //     LOG_ERROR("failed to allocate vertex buffer memory!");
        //     return;
        // }

        // vkBindBufferMemory(m_device->getDevice(), m_vertex_buffer, m_vertex_buffer_memory, 0);

        // void* data;
        // vkMapMemory(m_device->getDevice(), m_vertex_buffer_memory, 0, buffer_info.size, 0, &data);
        // memcpy(data, vertices.data(), static_cast<size_t>(buffer_info.size));
        // vkUnmapMemory(m_device->getDevice(), m_vertex_buffer_memory);

        // -------------------------------------------------------------

        // 使用新的GPU缓冲区工具函数创建顶点缓冲区
        size_t buffer_size = sizeof(vertices[0]) * vertices.size();

        VkBuffer       staging_buffer        = GPUBufferUtility::createBuffer(m_device->getDevice(), buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        VkDeviceMemory staging_buffer_memory = GPUBufferUtility::allocateBufferMemory(
            m_device->getDevice(), m_device->getPhysicalDevice(), staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        GPUBufferUtility::bindBufferMemory(m_device->getDevice(), staging_buffer, staging_buffer_memory);

        void* data;
        vkMapMemory(m_device->getDevice(), staging_buffer_memory, 0, buffer_size, 0, &data);
        memcpy(data, vertices.data(), buffer_size);
        vkUnmapMemory(m_device->getDevice(), staging_buffer_memory);

        // GPUBufferUtility::destroyBuffer(m_device->getDevice(), staging_buffer);
        // GPUBufferUtility::freeBufferMemory(m_device->getDevice(), m_device->getPhysicalDevice(), staging_buffer_memory);

        // 创建顶点缓冲区（使用设备本地内存以获得更好的性能）
        m_vertex_buffer =
            GPUBufferUtility::createBuffer(m_device->getDevice(), buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        // 分配设备本地内存（性能更好）
        m_vertex_buffer_memory =
            GPUBufferUtility::allocateBufferMemory(m_device->getDevice(), m_device->getPhysicalDevice(), m_vertex_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        GPUBufferUtility::bindBufferMemory(m_device->getDevice(), m_vertex_buffer, m_vertex_buffer_memory);
        GPUBufferUtility::copyBuffer(
            m_device->getDevice(), m_command_pool->getCommandPool(), m_device->getGraphicsQueue(), staging_buffer, m_vertex_buffer, buffer_size
        );

        vkDestroyBuffer(m_device->getDevice(), staging_buffer, nullptr);
        vkFreeMemory(m_device->getDevice(), staging_buffer_memory, nullptr);

        LOG_INFO("顶点缓冲区创建和上传完成，大小: {} 字节", buffer_size);
    }
} // namespace Piccolo
