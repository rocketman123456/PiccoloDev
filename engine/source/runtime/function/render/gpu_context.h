#pragma once

#include "runtime/core/base/macro.h"

#include <volk.h>
// #include <vulkan/vulkan.h>

namespace Piccolo
{
    class GPUContext
    {
    public:
        GPUContext();
        ~GPUContext();

        VkInstance getInstance() const { return m_instance; }

    private:
        void createInstance();
        void setupDebugMessenger();

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
        bool checkValidationLayerSupport();

        std::vector<const char*> getRequiredExtensions();

        VkInstance               m_instance;
        VkDebugUtilsMessengerEXT m_debug_messenger;

#ifdef NDEBUG
        const bool m_enable_validation_layers = false;
#else
        const bool m_enable_validation_layers = true;
#endif

        const std::vector<const char*> m_validation_layers = {
            "VK_LAYER_KHRONOS_validation",
        };

        // static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        //     VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        //     VkDebugUtilsMessageTypeFlagsEXT             messageType,
        //     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        //     void*                                       pUserData
        // )
        // {
        //     if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        //     {
        //         LOG_DEBUG("validation layer: {}", pCallbackData->pMessage);
        //     }
        //     else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        //     {
        //         LOG_INFO("validation layer: {}", pCallbackData->pMessage);
        //     }
        //     else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        //     {
        //         LOG_WARN("validation layer: {}", pCallbackData->pMessage);
        //     }
        //     else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        //     {
        //         LOG_ERROR("validation layer: {}", pCallbackData->pMessage);
        //     }
        //     else
        //     {
        //         LOG_WARN("validation layer: {}", pCallbackData->pMessage);
        //     }
        //     return VK_FALSE;
        // }
    };
} // namespace Piccolo
