#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace Piccolo
{
    class GPUDevice
    {
    public:
        GPUDevice(VkInstance instance);
        ~GPUDevice();

        VkDevice         getDevice() const { return device; }
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        VkQueue          getGraphicsQueue() const { return graphicsQueue; }
        uint32_t         getGraphicsQueueFamily() const { return graphicsQueueFamily; }

        bool supportsBindless() const { return bindlessSupported; }

    private:
        void pickPhysicalDevice(VkInstance instance);
        void createLogicalDevice();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        void queryBindlessSupport();

        VkPhysicalDevice physicalDevice      = VK_NULL_HANDLE;
        VkDevice         device              = VK_NULL_HANDLE;
        VkQueue          graphicsQueue       = VK_NULL_HANDLE;
        uint32_t         graphicsQueueFamily = UINT32_MAX;

        bool bindlessSupported = false;

        // Required device extensions
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//             VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
// #ifdef __APPLE__
//             "VK_KHR_portability_subset"
// #endif
        };
    };
} // namespace Piccolo
