#include "runtime/function/render/gpu_swap_chain.h"
#include "runtime/function/render/utils/gpu_utils.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/window_system.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
// #include <volk.h>
#include <GLFW/glfw3.h>

namespace Piccolo
{
    GPUSwapChain::GPUSwapChain(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface)
    {
        m_physical_device = physical_device;
        m_logical_device  = device;
        m_surface         = surface;

        createSwapchain(device, surface);
        createImageViews(device);
    }

    GPUSwapChain::~GPUSwapChain()
    {
        for (auto view : m_image_views)
        {
            vkDestroyImageView(m_logical_device, view, nullptr);
        }
        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
        }
    }

    VkSurfaceFormatKHR GPUSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
    {
        for (const auto& available_format : available_formats)
        {
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return available_format;
            }
        }
        return available_formats[0];
    }

    VkPresentModeKHR GPUSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
    {
        for (const auto& available_present_mode : available_present_modes)
        {
            if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return available_present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D GPUSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            auto* window = g_runtime_global_context.m_window_system->getWindow();

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actual_extent.width  = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actual_extent;
        }
    }

    void GPUSwapChain::createSwapchain(VkDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails swap_chain_support = querySwapChainSupport(m_physical_device, m_surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swap_chain_support.formats);
        VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swap_chain_support.present_modes);
        VkExtent2D         extent        = chooseSwapExtent(swap_chain_support.capabilities);

        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
        if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
        {
            image_count = swap_chain_support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount    = image_count;
        createInfo.imageFormat      = surfaceFormat.format;
        createInfo.imageColorSpace  = surfaceFormat.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(m_physical_device, m_surface);

        uint32_t queueFamilyIndices[] = {indices.graphics_family.value(), indices.present_family.value()};

        if (indices.graphics_family != indices.present_family)
        {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform   = swap_chain_support.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode    = presentMode;
        createInfo.clipped        = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_logical_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
        {
            LOG_ERROR("failed to create swap chain!")
        }

        vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, nullptr);
        m_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, m_images.data());

        LOG_INFO("swap chain image count: {}", image_count);

        m_image_format = surfaceFormat.format;
        m_extent       = extent;
    }

    void GPUSwapChain::createImageViews(VkDevice device)
    {
        m_image_views.resize(m_images.size());
        for (size_t i = 0; i < m_images.size(); i++)
        {
            VkImageViewCreateInfo viewInfo {};
            viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image                           = m_images[i];
            viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format                          = m_image_format;
            viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(m_logical_device, &viewInfo, nullptr, &m_image_views[i]) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to create image view for swapchain image view");
            }
        }
    }

} // namespace Piccolo
