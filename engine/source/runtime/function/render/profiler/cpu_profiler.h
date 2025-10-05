#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace Piccolo
{
    struct CPUTimestamp
    {
        std::string name;       // 时间戳名称
        double      elapsed_ms; // 经过的时间（毫秒）
        double      start_time; // 开始时间
        double      end_time;   // 结束时间
    };

    class CPUProfiler
    {
    public:
        CPUProfiler()  = default;
        ~CPUProfiler() = default;

        void initialize();
        void clear();

        void beginFrame(uint32_t frame_index);
        void endFrame(uint32_t frame_index);

        // 开始和结束性能测量
        void beginTimestamp(const char* name);
        void endTimestamp(const char* name);

        // 获取性能数据
        bool                hasValidData() const;
        uint32_t            getTimestampCount() const;
        const CPUTimestamp* getTimestamps() const;

        // 重置当前帧的查询
        void resetFrame(uint32_t frame_index);

    private:
        using TimePoint = std::chrono::high_resolution_clock::time_point;
        using Duration  = std::chrono::duration<double, std::milli>;

        std::vector<CPUTimestamp>                  m_current_timestamps;
        std::unordered_map<std::string, TimePoint> m_active_timestamps;
        uint32_t                                   m_current_frame = 0;
        bool                                       m_frame_active  = false;
        TimePoint                                  m_frame_start_time;
    };
} // namespace Piccolo
