#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace Piccolo
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
        std::optional<uint32_t> compute_family;

        bool isComplete() { return graphics_family.has_value() && present_family.has_value() && compute_family.has_value(); }
    };

    class GPUDevice
    {
    public:
        GPUDevice(VkInstance instance);
        ~GPUDevice();

        VkDevice         getDevice() const { return m_device; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physical_device; }
        VkSurfaceKHR     getSurface() const { return m_surface; }

        VkQueue  getGraphicsQueue() const { return m_graphics_queue; }
        VkQueue  getPresentQueue() const { return m_present_queue; }
        VkQueue  getComputeQueue() const { return m_compute_queue; }
        uint32_t getGraphicsQueueFamily() const { return m_graphics_queue_family; }
        uint32_t getPresentQueueFamily() const { return m_present_queue_family; }
        uint32_t getComputeQueueFamily() const { return m_compute_queue_family; }

        bool supportsBindless() const { return m_bindless_supported; }

    private:
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();

        // Utility
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        bool isDeviceSuitable(VkPhysicalDevice device);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        VkInstance m_instance; // from outside

        VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
        VkDevice         m_device          = VK_NULL_HANDLE;
        VkSurfaceKHR     m_surface;

        VkQueue m_graphics_queue = VK_NULL_HANDLE;
        VkQueue m_present_queue  = VK_NULL_HANDLE;
        VkQueue m_compute_queue  = VK_NULL_HANDLE;

        uint32_t m_graphics_queue_family = UINT32_MAX;
        uint32_t m_compute_queue_family  = UINT32_MAX;
        uint32_t m_present_queue_family  = UINT32_MAX;

        VkAllocationCallbacks* m_allocation_callbacks;

        bool m_bindless_supported = false;

#ifdef __APPLE__
        const bool m_enable_descriptor_indexing = false;
#else
        const bool m_enable_descriptor_indexing = true;
#endif

        // Required device extensions
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        // "VK_KHR_portability_subset",
#else
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
#endif
        };
    };
} // namespace Piccolo
