#include "runtime/function/render/gpu_sync_object.h"

#include "runtime/core/base/macro.h"

namespace Piccolo
{
    GPUSyncObject::GPUSyncObject(VkDevice device, uint32_t max_frames_in_flight)
    {
        m_device               = device;
        m_max_frames_in_flight = max_frames_in_flight;

        createSyncObjects();
    }

    GPUSyncObject::~GPUSyncObject()
    {
        for (size_t i = 0; i < m_max_frames_in_flight; i++)
        {
            vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
            vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
        }
    }

    void GPUSyncObject::createSyncObjects()
    {
        VkFenceCreateInfo fence_info {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // 初始化为已信号状态，避免第一次等待时卡死

        VkSemaphoreCreateInfo semaphore_info {};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // 为每个帧创建独立的同步对象
        m_image_available_semaphores.resize(m_max_frames_in_flight);
        m_render_finished_semaphores.resize(m_max_frames_in_flight);
        m_in_flight_fences.resize(m_max_frames_in_flight);

        for (size_t i = 0; i < m_max_frames_in_flight; i++)
        {
            if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS)
            {
                LOG_ERROR("failed to create image available semaphore for frame {}!", i);
            }
            if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS)
            {
                LOG_ERROR("failed to create render finished semaphore for frame {}!", i);
            }
            if (vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS)
            {
                LOG_ERROR("failed to create in flight fence for frame {}!", i);
            }
        }
    }
} // namespace Piccolo
