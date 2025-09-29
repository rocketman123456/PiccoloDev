#pragma once

namespace Piccolo
{
    // Define events as plain structs:
    struct PlayerDamaged
    {
        int player_id;
        int amount;
    };

    struct KeyPressed
    {
        int  key;
        bool repeat;
    };
} // namespace Piccolo
