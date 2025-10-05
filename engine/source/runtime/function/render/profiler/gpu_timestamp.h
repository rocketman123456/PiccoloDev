#pragma once

#include <volk.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Piccolo
{
    struct GPUTimestamp
    {
        uint32_t start;
        uint32_t end;

        double elapsed_ms;

        uint16_t parent_index;
        uint16_t depth;

        uint32_t color;
        uint32_t frame_index;

        const char* name;
    }; // struct GPUTimestamp

    class GPUTimestampManager
    {
    public:
        GPUTimestampManager() = default;
        ~GPUTimestampManager() { clear(); }

        void initialize(VkDevice device, VkPhysicalDevice physical_device, uint16_t queries_per_frame, uint16_t max_frames);
        void clear();

        bool     hasValidQueries() const;
        void     reset();
        uint32_t resolve(uint32_t current_frame, GPUTimestamp* timestamps_to_fill); // Returns the total queries for this frame.

        uint32_t push(uint32_t current_frame, const char* name); // Returns the timestamp query index.
        uint32_t pop(uint32_t current_frame);

        // 获取时间戳查询池
        VkQueryPool getQueryPool() const { return m_query_pool; }

        // 获取时间戳周期（纳秒）
        double getTimestampPeriod() const { return m_timestamp_period; }

    private:
        VkDevice    m_device     = VK_NULL_HANDLE;
        VkQueryPool m_query_pool = VK_NULL_HANDLE;

        std::vector<GPUTimestamp> m_timestamps;
        std::vector<uint64_t>     m_timestamps_data;

        // GPUTimestamp* m_timestamps      = nullptr;
        // uint64_t*     m_timestamps_data = nullptr;

        uint32_t m_queries_per_frame = 0;
        uint32_t m_max_frames        = 0;
        uint32_t m_current_query     = 0;
        uint32_t m_parent_index      = 0;
        uint32_t m_depth             = 0;

        double m_timestamp_period = 1.0; // GPU时间戳周期（纳秒）

        bool m_current_frame_resolved = false; // Used to query the GPU only once per frame if get_gpu_timestamps is called more than once per frame.
    };

} // namespace Piccolo
