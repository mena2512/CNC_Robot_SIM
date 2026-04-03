#pragma once
#include "Command.h"
#include <sstream>
#include <iomanip>

// ============================================================
//  LinearCommand — handles both G00 (rapid) and G01 (feed).
//
//  G00: moves the tool at rapid speed, no path geometry drawn.
//  G01: moves the tool at feed rate, pushes line segments.
//
//  Feed rate override is applied via robot.feed each frame,
//  so the caller can change it between frames freely.
// ============================================================
class LinearCommand : public Command
{
public:
    // type: 0 = G00 rapid,  1 = G01 cutting
    LinearCommand(sf::Vector2f target, int type, float feedOverride = -1.f)
        : m_target(target)
        , m_type(type)
        , m_feedOverride(feedOverride)
    {
    }

    void execute(Robot& robot, float dt) override
    {
        if (m_finished) return;

        // Apply feed word if this command carries one
        if (m_feedOverride > 0.f)
            robot.feed = m_feedOverride;

        float speed = (m_type == 0) ? RAPID_SPEED : robot.feed;
        float step = speed * dt;

        sf::Vector2f prev = robot.pos;
        float dist = vecLen(robot.pos, m_target);

        if (dist <= step || dist < 1e-5f)
        {
            // Snap exactly to target — eliminates float drift
            if (m_type == 1 && dist > 1e-5f)
                robot.addSegment(prev, m_target);

            robot.pos = m_target;
            m_finished = true;
            ++robot.commandsExecuted;
        }
        else
        {
            robot.pos += norm(m_target - robot.pos) * step;

            if (m_type == 1)
                robot.addSegment(prev, robot.pos);
        }
    }

    bool isFinished() const override { return m_finished; }

    std::string label() const override
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << (m_type == 0 ? "G00" : "G01")
            << " X" << m_target.x
            << " Y" << m_target.y;
        return ss.str();
    }

private:
    static constexpr float RAPID_SPEED = 400.f;  // units/sec for G00

    sf::Vector2f m_target;
    int          m_type;
    float        m_feedOverride;
    bool         m_finished{ false };
};