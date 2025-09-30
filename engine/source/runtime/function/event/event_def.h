#pragma once

namespace Piccolo
{
    // Define events as plain structs:
    struct MouseMove
    {
        double x;
        double y;
        double dx;
        double dy;
        int    key;
        bool   repeat;
    };

    struct KeyPressed
    {
        int  key;
        bool repeat;
    };
} // namespace Piccolo
