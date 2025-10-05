#include "runtime/function/render/profiler/gpu_profiler.h"

#include "runtime/core/base/macro.h"

namespace Piccolo
{
    void GPUProfiler::initialize(VkDevice device, VkPhysicalDevice physical_device, uint16_t queries_per_frame, uint16_t max_frames)
    {
        m_device            = device;
        m_timestamp_manager = std::make_unique<GPUTimestampManager>();
        m_timestamp_manager->initialize(device, physical_device, queries_per_frame, max_frames);

        // 预分配时间戳缓冲区
        m_current_timestamps.resize(queries_per_frame);

        LOG_INFO("GPU Profiler initialized with {} queries per frame, {} max frames", queries_per_frame, max_frames);
    }

    void GPUProfiler::clear()
    {
        if (m_timestamp_manager)
        {
            m_timestamp_manager->clear();
            m_timestamp_manager.reset();
        }

        m_current_timestamps.clear();
        m_device        = VK_NULL_HANDLE;
        m_current_frame = 0;
        m_frame_active  = false;
    }

    void GPUProfiler::beginFrame(uint32_t frame_index)
    {
        m_current_frame = frame_index;
        m_frame_active  = true;
        m_timestamp_manager->reset();
    }

    void GPUProfiler::resetQueryPool(VkCommandBuffer command_buffer, uint32_t frame_index)
    {
        if (!m_frame_active || frame_index != m_current_frame)
        {
            return;
        }

        // 重置当前帧的所有查询
        uint32_t first_query = frame_index * 64 * 2; // 每帧64个查询，每个查询2个时间戳
        uint32_t query_count = 64 * 2;
        vkCmdResetQueryPool(command_buffer, m_timestamp_manager->getQueryPool(), first_query, query_count);
    }

    void GPUProfiler::endFrame(uint32_t frame_index)
    {
        if (!m_frame_active || frame_index != m_current_frame)
        {
            return;
        }

        // 解析当前帧的时间戳数据
        uint32_t timestamp_count = m_timestamp_manager->resolve(frame_index, m_current_timestamps.data());
        m_current_timestamps.resize(timestamp_count);

        m_frame_active = false;
    }

    void GPUProfiler::beginTimestamp(VkCommandBuffer command_buffer, uint32_t frame_index, const char* name)
    {
        if (!m_frame_active || frame_index != m_current_frame)
        {
            return;
        }

        uint32_t query_index = m_timestamp_manager->push(frame_index, name);
        if (query_index != UINT32_MAX)
        {
            // 记录开始时间戳
            uint32_t timestamp_index = frame_index * 64 * 2 + query_index * 2; // 每帧64个查询，每个查询2个时间戳
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_timestamp_manager->getQueryPool(), timestamp_index);
        }
    }

    void GPUProfiler::endTimestamp(VkCommandBuffer command_buffer, uint32_t frame_index)
    {
        if (!m_frame_active || frame_index != m_current_frame)
        {
            return;
        }

        uint32_t query_index = m_timestamp_manager->pop(frame_index);
        if (query_index != UINT32_MAX)
        {
            // 记录结束时间戳
            uint32_t timestamp_index = frame_index * 64 * 2 + query_index * 2 + 1; // 每帧64个查询，每个查询2个时间戳
            vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_timestamp_manager->getQueryPool(), timestamp_index);
        }
    }

    bool GPUProfiler::hasValidData() const { return !m_current_timestamps.empty() && m_timestamp_manager->hasValidQueries(); }

    uint32_t GPUProfiler::getTimestampCount() const { return static_cast<uint32_t>(m_current_timestamps.size()); }

    const GPUTimestamp* GPUProfiler::getTimestamps() const { return m_current_timestamps.empty() ? nullptr : m_current_timestamps.data(); }

    void GPUProfiler::resetFrame(uint32_t frame_index)
    {
        if (frame_index == m_current_frame)
        {
            m_timestamp_manager->reset();
            m_current_timestamps.clear();
        }
    }
} // namespace Piccolo
