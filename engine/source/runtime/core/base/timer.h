#pragma once

#include <chrono>
#include <iostream>

class GameTimer
{
    using std::chrono;

public:
    GameTimer()  = default;
    ~GameTimer() = default;

    void pause() { m_paused = true; }

    void resume() { m_paused = false; }

    void tick(double dt)
    {
        if (m_paused)
            return;

        m_current_time += dt;
    }

private:
    bool   m_paused {false};
    double m_current_time {0.0};
};
