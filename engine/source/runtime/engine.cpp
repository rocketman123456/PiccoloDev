#include "runtime/engine.h"

#include "runtime/core/base/macro.h"

#include "runtime/core/log/log_system.h"
#include "runtime/platform/file_service/file_service.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/event/event_system.h"

#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

#include "runtime/function/profiler/cpu_profiler.h"
#include "runtime/function/profiler/gpu_profiler.h"

namespace Piccolo
{
    bool g_is_editor_mode {false};
    bool g_is_game_mode {false};

    std::unordered_set<std::string> g_editor_tick_component_types {};

    const float PiccoloEngine::s_fps_alpha = 1.f / 100.f;

    void PiccoloEngine::startEngine(const std::string& config_file_path)
    {
        g_runtime_global_context.startSystems(config_file_path);

        LOG_INFO("engine start");
    }

    void PiccoloEngine::shutdownEngine()
    {
        LOG_INFO("engine shutdown");

        g_runtime_global_context.shutdownSystems();
    }

    void PiccoloEngine::initialize()
    {
        //
    }

    void PiccoloEngine::clear()
    {
        //
    }

    void PiccoloEngine::run()
    {
        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
        ASSERT(window_system);

        while (!window_system->shouldClose())
        {
            const float delta_time = calculateDeltaTime();
            tickOneFrame(delta_time);
        }
    }

    float PiccoloEngine::calculateDeltaTime()
    {
        float delta_time;
        {
            using namespace std::chrono;

            steady_clock::time_point tick_time_point = steady_clock::now();
            duration<float>          time_span       = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);
            delta_time                               = time_span.count();

            m_last_tick_time_point = tick_time_point;
        }
        return delta_time;
    }

    bool PiccoloEngine::tickOneFrame(float delta_time)
    {
        logicalTick(delta_time);
        calculateFPS(delta_time);

        g_runtime_global_context.m_window_system->pollEvents();
        g_runtime_global_context.m_window_system->setTitle(std::string("Piccolo - " + std::to_string(getFPS()) + " FPS").c_str());

        rendererTick(delta_time);

        LOG_INFO("=== Frame {} completed ===", m_frame_count);
        if (g_runtime_global_context.m_gpu_profiler->hasValidData() || g_runtime_global_context.m_cpu_profiler->hasValidData())
        {
            logProfilerData();
        }
        else
        {
            LOG_INFO("No profiler data available for frame {}", m_frame_count);
        }

        const bool should_window_close = g_runtime_global_context.m_window_system->shouldClose();
        return !should_window_close;
    }

    void PiccoloEngine::logicalTick(float delta_time)
    {
        // Update event handle
        g_runtime_global_context.m_event_system->tick(delta_time);
    }

    bool PiccoloEngine::rendererTick(float delta_time)
    {
        g_runtime_global_context.m_render_system->tick(delta_time);

        return true;
    }

    void PiccoloEngine::calculateFPS(float delta_time)
    {
        m_frame_count++;

        if (m_frame_count == 1)
        {
            m_average_duration = delta_time;
        }
        else
        {
            m_average_duration = m_average_duration * (1 - s_fps_alpha) + delta_time * s_fps_alpha;
        }

        m_fps = static_cast<int>(1.f / m_average_duration);
    }

    void PiccoloEngine::logProfilerData()
    {
        bool has_gpu_data = g_runtime_global_context.m_gpu_profiler->hasValidData();
        bool has_cpu_data = g_runtime_global_context.m_cpu_profiler->hasValidData();

        if (!has_gpu_data && !has_cpu_data)
        {
            LOG_DEBUG("=== Profiler Data (Frame {}) - No valid data ===", m_frame_count);
            return;
        }

        LOG_DEBUG("=== Profiler Data (Frame {}) ===", m_frame_count);

        // 输出GPU profiler数据
        if (has_gpu_data)
        {
            const GPUTimestamp* gpu_timestamps = g_runtime_global_context.m_gpu_profiler->getTimestamps();
            uint32_t            gpu_count      = g_runtime_global_context.m_gpu_profiler->getTimestampCount();

            LOG_DEBUG("--- GPU Profiler Data ({} timestamps) ---", gpu_count);
            for (uint32_t i = 0; i < gpu_count; ++i)
            {
                const GPUTimestamp& timestamp = gpu_timestamps[i];
                LOG_DEBUG(
                    "  GPU {}: {:.3f} ms (start: {}, end: {})",
                    timestamp.name ? timestamp.name : "Unknown",
                    timestamp.elapsed_ms,
                    timestamp.start,
                    timestamp.end
                );
            }
        }
        else
        {
            LOG_DEBUG("--- GPU Profiler Data - No valid data ---");
        }

        // 输出CPU profiler数据
        if (has_cpu_data)
        {
            const CPUTimestamp* cpu_timestamps = g_runtime_global_context.m_cpu_profiler->getTimestamps();
            uint32_t            cpu_count      = g_runtime_global_context.m_cpu_profiler->getTimestampCount();

            LOG_DEBUG("--- CPU Profiler Data ({} timestamps) ---", cpu_count);
            for (uint32_t i = 0; i < cpu_count; ++i)
            {
                const CPUTimestamp& timestamp = cpu_timestamps[i];
                // LOG_INFO("  CPU {}: {:.3f} ms (start: {:.3f}, end: {:.3f})", timestamp.name, timestamp.elapsed_ms, timestamp.start_time, timestamp.end_time);
                LOG_DEBUG("  CPU {}: {:.3f} ms", timestamp.name, timestamp.elapsed_ms);
            }
        }
        else
        {
            LOG_DEBUG("--- CPU Profiler Data - No valid data ---");
        }

        LOG_DEBUG("=== End Profiler Data ===");
    }
} // namespace Piccolo
