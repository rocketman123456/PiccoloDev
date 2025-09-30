#include "runtime/function/render/gpu_context.h"

#include "runtime/core/base/macro.h"

namespace Piccolo
{
    GPUContext::GPUContext() { createInstance(); }
    GPUContext::~GPUContext() { vkDestroyInstance(m_instance, nullptr); }

    void GPUContext::createInstance()
    {
        VkApplicationInfo appInfo {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "Simple Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "No Engine";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo {};
        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create Vulkan instance");
        }
    }
} // namespace Piccolo
