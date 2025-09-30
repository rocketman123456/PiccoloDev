#include "runtime/function/render/gpu_device.h"

#include "runtime/core/base/macro.h"

#include <iostream>
#include <set>

namespace Piccolo
{
    GPUDevice::GPUDevice(VkInstance instance)
    {
        pickPhysicalDevice(instance);
        createLogicalDevice();
        queryBindlessSupport();
    }

    GPUDevice::~GPUDevice()
    {
        if (device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(device, nullptr);
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

        std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& ext : availableExtensions)
        {
            required.erase(ext.extensionName);
        }

        return required.empty();
    }

    void GPUDevice::pickPhysicalDevice(VkInstance instance)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            LOG_ERROR("Failed: no Vulkan-capable GPU found");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& dev : devices)
        {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, families.data());

            for (uint32_t i = 0; i < queueFamilyCount; i++)
            {
                if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    if (checkDeviceExtensionSupport(dev))
                    {
                        physicalDevice      = dev;
                        graphicsQueueFamily = i;
                        LOG_INFO("Selected GPU with graphics + swapchain support.")
                        return;
                    }
                }
            }
        }

        LOG_ERROR("Failed: no suitable GPU with required extensions found");
    }

    void GPUDevice::createLogicalDevice()
    {
        float                   queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo {};
        queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = graphicsQueueFamily;
        queueInfo.queueCount       = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures2 features2 {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures {};
        indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        features2.pNext        = &indexingFeatures;

        vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

        VkDeviceCreateInfo createInfo {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount    = 1;
        createInfo.pQueueCreateInfos       = &queueInfo;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pNext                   = &indexingFeatures; // enable descriptor indexing if available

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create logical device");
        }

        vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    }

    void GPUDevice::queryBindlessSupport()
    {
        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures {};
        indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

        VkPhysicalDeviceFeatures2 features2 {};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &indexingFeatures;

        vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

        bindlessSupported = indexingFeatures.runtimeDescriptorArray && indexingFeatures.shaderSampledImageArrayNonUniformIndexing &&
                            indexingFeatures.descriptorBindingPartiallyBound;

        if (bindlessSupported)
        {
            LOG_DEBUG("Bindless (descriptor indexing) is supported.");
        }
        else
        {
            LOG_WARN("Bindless not supported on this GPU.");
        }
    }
} // namespace Piccolo
