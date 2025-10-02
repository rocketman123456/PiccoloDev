#include "runtime/function/render/profiler/gpu_timestamp.h"

#include "runtime/core/base/macro.h"

#include <set>

namespace Piccolo
{
    void GPUTimestampManager::initialize(uint16_t queries_per_frame, uint16_t max_frames)
    {
        //
    }
    void GPUTimestampManager::clear()
    {
        //
    }

    bool GPUTimestampManager::hasValidQueries() const
    {
        //
        return false;
    }
    void GPUTimestampManager::reset()
    {
        //
    }
    uint32_t GPUTimestampManager::resolve(uint32_t current_frame, GPUTimestamp* timestamps_to_fill)
    {
        //
        return 0;
    }

    uint32_t GPUTimestampManager::push(uint32_t current_frame, const char* name)
    {
        //
        return 0;
    }
    uint32_t GPUTimestampManager::pop(uint32_t current_frame)
    {
        //
        return 0;
    }

} // namespace Piccolo
