#pragma once

#include <volk.h>

namespace Piccolo
{
    uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}