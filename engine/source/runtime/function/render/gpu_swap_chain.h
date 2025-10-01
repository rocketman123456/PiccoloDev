#pragma once

#include "runtime/function/render/utils/gpu_utils.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#include <vector>

namespace Piccolo
{

    class GPUSwapChain
    {
    public:
        GPUSwapChain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface);
        ~GPUSwapChain();

        VkSwapchainKHR getSwapchain() const { return m_swapchain; }
        VkFormat       getImageFormat() const { return m_image_format; }
        VkExtent2D     getExtent() const { return m_extent; }

        const std::vector<VkImage>&     getImages() const { return m_images; }
        const std::vector<VkImageView>& getImageViews() const { return m_image_views; }

    private:
        void createSwapchain(VkDevice device, VkSurfaceKHR surface);
        void createImageViews(VkDevice device);

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
        VkSurfaceFormatKHR      chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
        VkPresentModeKHR        chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
        VkExtent2D              chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VkPhysicalDevice m_physical_device;
        VkDevice         m_logical_device;
        VkSurfaceKHR     m_surface;

        VkSwapchainKHR           m_swapchain = VK_NULL_HANDLE;
        std::vector<VkImage>     m_images;
        std::vector<VkImageView> m_image_views;
        VkFormat                 m_image_format;
        VkExtent2D               m_extent;
    };
} // namespace Piccolo
