#pragma once

#include <volk.h>
// #include <vulkan/vulkan.h>
#include <vector>

namespace Piccolo
{
    class GPUSyncObject
    {
    public:
        explicit GPUSyncObject(VkDevice device, uint32_t max_frames_in_flight = 2);
        ~GPUSyncObject();

        // TODO : make general SyncObject class
        VkSemaphore getImageAvailableSemaphore(uint32_t frame_index) const { return m_image_available_semaphores[frame_index]; }
        VkSemaphore getRenderFinishedSemaphore(uint32_t frame_index) const { return m_render_finished_semaphores[frame_index]; }
        VkFence     getInFlightFence(uint32_t frame_index) const { return m_in_flight_fences[frame_index]; }

        uint32_t getMaxFramesInFlight() const { return m_max_frames_in_flight; }

    private:
        void createSyncObjects();

        VkDevice                 m_device;
        uint32_t                 m_max_frames_in_flight;
        std::vector<VkSemaphore> m_image_available_semaphores;
        std::vector<VkSemaphore> m_render_finished_semaphores;
        std::vector<VkFence>     m_in_flight_fences;
    };
} // namespace Piccolo