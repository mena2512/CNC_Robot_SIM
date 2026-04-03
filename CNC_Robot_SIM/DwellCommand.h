#pragma once
#include "Command.h"
#include <sstream>
#include <iomanip>

// ============================================================
//  DwellCommand — G04 P<seconds>
//
//  Holds the tool at its current position for a fixed duration.
//  No motion, no geometry — pure time delay.
//  Useful for demos (pause between shapes) and real G-code
//  where the spindle needs time to reach speed.
//
//  Example in file:   G04 P1.5   (dwell 1.5 seconds)
// ============================================================
class DwellCommand : public Command
{
public:
    explicit DwellCommand(float seconds)
        : m_duration(seconds)
    {
    }

    void execute(Robot& /*robot*/, float dt) override
    {
        if (m_finished) return;
        m_elapsed += dt;
        if (m_elapsed >= m_duration)
            m_finished = true;
    }

    bool isFinished() const override { return m_finished; }

    std::string label() const override
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "G04 P" << m_duration
            << "  (" << std::min(m_elapsed, m_duration) << "s)";
        return ss.str();
    }

    float progress() const
    {
        return (m_duration > 0.f)
            ? std::min(m_elapsed / m_duration, 1.f)
            : 1.f;
    }

private:
    float m_duration;
    float m_elapsed{ 0.f };
    bool  m_finished{ false };
};