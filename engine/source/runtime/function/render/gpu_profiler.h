#pragma once

namespace Piccolo
{
    class GPUProfiler
    {
    public:
        void initialize();
        void clear();

        void update();
    };
}