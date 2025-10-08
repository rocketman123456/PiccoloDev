#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/utils/gpu_utils.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/window_system.h"

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
// #include <volk.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>

namespace Piccolo
{
    GPUDevice::GPUDevice(VkInstance instance)
    {
        m_instance = instance;

        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    GPUDevice::~GPUDevice()
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_device, nullptr);
        }

        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
    }

    void GPUDevice::createSurface()
    {
        auto* window = g_runtime_global_context.m_window_system->getWindow();

        if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS)
        {
            LOG_ERROR("failed to create window surface!");
        }
    }

    bool GPUDevice::checkDeviceExtensionSupport(VkPhysicalDevice dev)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

        LOG_DEBUG("support extensions:")
        for (const auto& ext : availableExtensions)
        {
            LOG_DEBUG("  {}", ext.extensionName);
        }

        std::set<std::string> required(m_device_extensions.begin(), m_device_extensions.end());

        for (const auto& ext : availableExtensions)
        {
            required.erase(ext.extensionName);
        }

        return required.empty();
    }

    bool GPUDevice::isDeviceSuitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
    {
        auto indices = findQueueFamilies(physical_device, surface);
        auto support = checkDeviceExtensionSupport(physical_device);

        bool swap_chain_adequate = false;
        if (support)
        {
            SwapChainSupportDetails swap_chain_support = querySwapChainSupport(physical_device, surface);
            swap_chain_adequate                        = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
        }

        return indices.isComplete() && support && swap_chain_adequate;
    }

    void GPUDevice::pickPhysicalDevice()
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
        if (device_count == 0)
        {
            LOG_ERROR("Failed: no Vulkan-capable GPU found");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device, m_surface))
            {
                m_physical_device = device;
                LOG_INFO("Selected GPU with support.")
                break;
            }
        }

        if (m_physical_device == VK_NULL_HANDLE)
        {
            LOG_ERROR("Failed: no suitable GPU with required extensions found");
        }
    }

    void GPUDevice::createLogicalDevice()
    {
        auto indices = findQueueFamilies(m_physical_device, m_surface);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t>                   unique_queue_families = {
            indices.graphics_family.value(),
            indices.present_family.value(),
        };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queueCreateInfo {};
            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount       = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceCreateInfo createInfo {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(m_device_extensions.size());
        createInfo.ppEnabledExtensionNames = m_device_extensions.data();
        createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos       = queueCreateInfos.data();

        // 启用hostQueryReset功能以支持vkResetQueryPool
        VkPhysicalDeviceHostQueryResetFeaturesEXT host_query_reset_features {};
        host_query_reset_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT;
        host_query_reset_features.hostQueryReset = VK_TRUE;

        if (m_enable_descriptor_indexing)
        {
            VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures {};
            indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
            indexingFeatures.pNext = &host_query_reset_features; // 链接到hostQueryReset功能

            VkPhysicalDeviceFeatures2 features2 {};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features2.pNext = &indexingFeatures;
            vkGetPhysicalDeviceFeatures2(m_physical_device, &features2);

            createInfo.pNext = &indexingFeatures; // enable descriptor indexing if available

            m_bindless_supported = indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray;
            if (m_bindless_supported)
            {
                LOG_DEBUG("Bindless (descriptor indexing) is supported.");
            }
            else
            {
                LOG_WARN("Bindless not supported on this GPU.");
            }
        }
        else
        {
            createInfo.pNext = &host_query_reset_features; // 即使没有descriptor indexing也启用hostQueryReset

            m_bindless_supported = false;
            LOG_WARN("Bindless not supported on this GPU.");
        }

        if (vkCreateDevice(m_physical_device, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create logical device");
        }

        volkLoadDevice(m_device);

        m_graphics_queue_family = indices.graphics_family.value();
        m_compute_queue_family  = indices.compute_family.value();
        m_present_queue_family  = indices.present_family.value();

        vkGetDeviceQueue(m_device, indices.graphics_family.value(), 0, &m_graphics_queue);
        vkGetDeviceQueue(m_device, indices.present_family.value(), 0, &m_present_queue);
        vkGetDeviceQueue(m_device, indices.compute_family.value(), 0, &m_compute_queue);
    }
} // namespace Piccolo
