#pragma once

#include <cstdint>

namespace Piccolo
{
    class ApplicationTickData
    {
    public:
        uint64_t tick_count;
        double total_time;

        double fps;
        double dt;

        // Update persecond fps this frame or not.
        bool   should_update_per_second;
        double fps_updated_per_second;
    };
} // namespace Piccolo
