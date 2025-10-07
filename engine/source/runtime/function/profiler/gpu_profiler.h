#pragma once

#include "runtime/function/profiler/gpu_timestamp.h"

#include <volk.h>

#include <memory>
#include <vector>

namespace Piccolo
{
    class GPUProfiler
    {
    public:
        GPUProfiler() = default;
        ~GPUProfiler() { clear(); }

        void initialize(VkDevice device, VkPhysicalDevice physical_device, uint16_t queries_per_frame = 64, uint16_t max_frames = 3);
        void clear();

        void beginFrame(uint32_t frame_index);
        void endFrame(uint32_t frame_index);

        // 开始和结束性能测量
        void beginTimestamp(VkCommandBuffer command_buffer, uint32_t frame_index, const char* name);
        void endTimestamp(VkCommandBuffer command_buffer, uint32_t frame_index);
        
        // 重置查询池
        void resetQueryPool(VkCommandBuffer command_buffer, uint32_t frame_index);

        // 获取性能数据
        bool                hasValidData() const;
        uint32_t            getTimestampCount() const;
        const GPUTimestamp* getTimestamps() const;

        // 重置当前帧的查询
        void resetFrame(uint32_t frame_index);

    private:
        VkDevice                             m_device = VK_NULL_HANDLE;
        std::unique_ptr<GPUTimestampManager> m_timestamp_manager;

        uint32_t m_queries_per_frame {0};
        uint32_t m_max_frames {0};

        std::vector<GPUTimestamp> m_current_timestamps;
        uint32_t                  m_current_frame = 0;
        bool                      m_frame_active  = false;
    };
} // namespace Piccolo