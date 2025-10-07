#include "runtime/function/profiler/cpu_profiler.h"

#include "runtime/core/base/macro.h"

#include <chrono>

namespace Piccolo
{
    void CPUProfiler::initialize()
    {
        m_current_timestamps.clear();
        m_active_timestamps.clear();
        m_current_frame = 0;
        m_frame_active  = false;

        LOG_INFO("CPU Profiler initialized");
    }

    void CPUProfiler::clear()
    {
        m_current_timestamps.clear();
        m_active_timestamps.clear();
        m_current_frame = 0;
        m_frame_active  = false;
    }

    void CPUProfiler::beginFrame(uint32_t frame_index)
    {
        m_current_frame    = frame_index;
        m_frame_active     = true;
        m_frame_start_time = std::chrono::high_resolution_clock::now();

        // 清除上一帧的时间戳数据
        m_current_timestamps.clear();
        m_active_timestamps.clear();
    }

    void CPUProfiler::endFrame(uint32_t frame_index)
    {
        if (!m_frame_active || frame_index != m_current_frame)
        {
            return;
        }

        // 结束所有未完成的时间戳
        for (auto& [name, start_time] : m_active_timestamps)
        {
            auto     end_time = std::chrono::high_resolution_clock::now();
            Duration duration = end_time - start_time;

            CPUTimestamp timestamp;
            timestamp.name       = name;
            timestamp.elapsed_ms = duration.count();
            timestamp.start_time = std::chrono::duration<double, std::milli>(start_time.time_since_epoch()).count();
            timestamp.end_time   = std::chrono::duration<double, std::milli>(end_time.time_since_epoch()).count();

            m_current_timestamps.push_back(timestamp);
        }

        m_active_timestamps.clear();
        m_frame_active = false;
    }

    void CPUProfiler::beginTimestamp(const char* name)
    {
        if (!m_frame_active)
        {
            return;
        }

        std::string timestamp_name          = name ? name : "Unknown";
        m_active_timestamps[timestamp_name] = std::chrono::high_resolution_clock::now();
    }

    void CPUProfiler::endTimestamp(const char* name)
    {
        if (!m_frame_active)
        {
            return;
        }

        std::string timestamp_name = name ? name : "Unknown";
        auto        it             = m_active_timestamps.find(timestamp_name);
        if (it == m_active_timestamps.end())
        {
            LOG_ERROR("CPU Profiler: No active timestamp found for '{}'", timestamp_name);
            return;
        }

        auto     end_time = std::chrono::high_resolution_clock::now();
        Duration duration = end_time - it->second;

        CPUTimestamp timestamp;
        timestamp.name       = timestamp_name;
        timestamp.elapsed_ms = duration.count();
        timestamp.start_time = std::chrono::duration<double, std::milli>(it->second.time_since_epoch()).count();
        timestamp.end_time   = std::chrono::duration<double, std::milli>(end_time.time_since_epoch()).count();

        m_current_timestamps.push_back(timestamp);
        m_active_timestamps.erase(it);
    }

    bool CPUProfiler::hasValidData() const { return !m_current_timestamps.empty(); }

    uint32_t CPUProfiler::getTimestampCount() const { return static_cast<uint32_t>(m_current_timestamps.size()); }

    const CPUTimestamp* CPUProfiler::getTimestamps() const { return m_current_timestamps.empty() ? nullptr : m_current_timestamps.data(); }

    void CPUProfiler::resetFrame(uint32_t frame_index)
    {
        if (frame_index == m_current_frame)
        {
            m_current_timestamps.clear();
            m_active_timestamps.clear();
        }
    }
} // namespace Piccolo
