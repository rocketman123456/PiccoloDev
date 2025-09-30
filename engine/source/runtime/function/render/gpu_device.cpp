#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/utils/render_utils.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/window_system.h"

// #define VK_NO_PROTOTYPES
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
            throw std::runtime_error("failed to create window surface!");
        }
    }

    bool GPUDevice::checkDeviceExtensionSupport(VkPhysicalDevice dev)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

        // LOG_DEBUG("support extensions:")
        // for (const auto& ext : availableExtensions)
        // {
        //     LOG_DEBUG("  {}", ext.extensionName);
        // }

        std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& ext : availableExtensions)
        {
            required.erase(ext.extensionName);
        }

        return required.empty();
    }

    QueueFamilyIndices GPUDevice::findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }

            if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                indices.compute_family = i;
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support);

            if (present_support)
            {
                indices.present_family = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    bool GPUDevice::isDeviceSuitable(VkPhysicalDevice physical_device)
    {
        auto indices = findQueueFamilies(physical_device);
        auto support = checkDeviceExtensionSupport(physical_device);

        return indices.isComplete() && support;
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
            if (isDeviceSuitable(device))
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
        auto indices = findQueueFamilies(m_physical_device);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t>                   uniqueQueueFamilies = {
            indices.graphics_family.value(),
            indices.present_family.value(),
        };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo {};
            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount       = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // float                   queuePriority = 1.0f;
        // VkDeviceQueueCreateInfo queueInfo {};
        // queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // queueInfo.queueFamilyIndex = m_graphics_queue_family;
        // queueInfo.queueCount       = 1;
        // queueInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo createInfo {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos       = queueCreateInfos.data();

        if (m_enable_descriptor_indexing)
        {
            VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures {};
            indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
            indexingFeatures.pNext = nullptr;

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
            createInfo.pNext = nullptr;

            m_bindless_supported = false;
            LOG_WARN("Bindless not supported on this GPU.");
        }

        if (vkCreateDevice(m_physical_device, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create logical device");
        }

        m_graphics_queue_family = indices.graphics_family.value();
        m_compute_queue_family  = indices.compute_family.value();
        m_present_queue_family  = indices.present_family.value();

        vkGetDeviceQueue(m_device, indices.graphics_family.value(), 0, &m_graphics_queue);
        vkGetDeviceQueue(m_device, indices.present_family.value(), 0, &m_present_queue);
        vkGetDeviceQueue(m_device, indices.compute_family.value(), 0, &m_compute_queue);
    }
} // namespace Piccolo
