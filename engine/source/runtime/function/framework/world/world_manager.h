#pragma once

namespace Piccolo
{
    class WorldManager
    {
    public:
        WorldManager()  = default;
        ~WorldManager() = default;

        void initialize();
        void finalize();
    };
} // namespace Piccolo