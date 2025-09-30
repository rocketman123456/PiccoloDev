#pragma once
#include <vector>
#include <vulkan/vulkan.h>

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

        VkInstance m_instance;
    };
} // namespace Piccolo
