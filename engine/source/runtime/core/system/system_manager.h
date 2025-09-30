#pragma once

#include "runtime/core/system/subsystem.h"

namespace Piccolo
{
    class SystemManager
    {
    public:
        SystemManager()  = default;
        ~SystemManager() = default;

        void initialize();
        void clear();
    };
} // namespace Piccolo
