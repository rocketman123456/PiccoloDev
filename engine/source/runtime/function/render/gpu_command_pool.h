#pragma once

#include <volk.h>
// #include <vulkan/vulkan.h>
#include <vector>

namespace Piccolo
{
    class GPUCommandPool
    {
    public:
        explicit GPUCommandPool(VkDevice device, uint32_t max_frames_in_flight = 2);
        ~GPUCommandPool();

        VkCommandBuffer getCommandBuffer(uint32_t frame_index) const { return m_command_buffers[frame_index]; }
        VkCommandPool   getCommandPool() const { return m_command_pool; }

        void recordCommandBuffer(VkCommandBuffer command_buffer, int image_index);

    private:
        void createCommandPool();
        void createCommandBuffers();

        VkDevice      m_device;
        uint32_t      m_max_frames_in_flight;
        VkCommandPool m_command_pool;

        std::vector<VkCommandBuffer> m_command_buffers;
    };
} // namespace Piccolo
