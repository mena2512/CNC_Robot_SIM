#pragma once
#include "Command.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// ============================================================
//  ArcCommand — handles G02 (CW) and G03 (CCW).
//
//  All arc state (startAngle, totalAngle, travel, prevPos)
//  lives inside this object — no external arcInit flag needed.
//
//  The arc is integrated in polar coordinates each frame:
//    dAngle = (feed / radius) * dt
//    pos    = center + radius * (cos(angle), sin(angle))
//
//  Each frame pushes one segment (prevArcPos → newPos) into
//  robot.path using robot.addSegment(), so rendering is automatic.
// ============================================================
class ArcCommand : public Command
{
public:
    // type: 2 = G02 CW,  3 = G03 CCW
    // centerOffset: I,J from the G-code line (offset from current pos to center)
    ArcCommand(sf::Vector2f target, sf::Vector2f centerOffset, int type, float feedOverride = -1.f)
        : m_target(target)
        , m_centerOffset(centerOffset)
        , m_type(type)
        , m_feedOverride(feedOverride)
    {
    }

    void execute(Robot& robot, float dt) override
    {
        if (m_finished) return;

        // Apply feed word once
        if (m_feedOverride > 0.f)
            robot.feed = m_feedOverride;

        // --- Initialise on first call ---
        if (!m_initialised)
        {
            m_center = robot.pos + m_centerOffset;
            m_radius = vecLen(m_center, robot.pos);
            if (m_radius < 1e-4f) m_radius = 1e-4f;

            sf::Vector2f sv = robot.pos - m_center;
            sf::Vector2f ev = m_target - m_center;

            m_startAngle = normAngle(std::atan2(sv.y, sv.x));
            float endAng = normAngle(std::atan2(ev.y, ev.x));

            m_dir = (m_type == 2) ? -1 : 1;   // G02 CW = -1,  G03 CCW = +1

            // Angular span — always positive, represents the arc sweep
            if (m_dir == 1)
                m_totalAngle = normAngle(endAng - m_startAngle);
            else
                m_totalAngle = normAngle(m_startAngle - endAng);

            // Full-circle fallback (start == end point)
            if (m_totalAngle < 1e-4f)
                m_totalAngle = 6.2831853f;

            m_travel = 0.f;
            m_prevPos = robot.pos;
            m_initialised = true;
        }

        // --- Angular integration ---
        float dA = (robot.feed / m_radius) * dt;
        m_travel += dA;

        if (m_travel >= m_totalAngle)
        {
            // Clamp to exact endpoint
            robot.addSegment(m_prevPos, m_target);
            robot.pos = m_target;
            m_finished = true;
            ++robot.commandsExecuted;
        }
        else
        {
            float ang = m_startAngle + m_dir * m_travel;
            sf::Vector2f newPos = {
                m_center.x + m_radius * std::cos(ang),
                m_center.y + m_radius * std::sin(ang)
            };
            robot.addSegment(m_prevPos, newPos);
            m_prevPos = newPos;
            robot.pos = newPos;
        }
    }

    bool isFinished() const override { return m_finished; }

    std::string label() const override
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << (m_type == 2 ? "G02" : "G03")
            << " X" << m_target.x
            << " Y" << m_target.y
            << " I" << m_centerOffset.x
            << " J" << m_centerOffset.y;
        return ss.str();
    }

private:
    sf::Vector2f m_target;
    sf::Vector2f m_centerOffset;
    int          m_type;
    float        m_feedOverride;

    // Arc state — fully encapsulated here, never leaks out
    bool         m_initialised{ false };
    bool         m_finished{ false };
    sf::Vector2f m_center;
    sf::Vector2f m_prevPos;
    float        m_radius{ 1.f };
    float        m_startAngle{ 0.f };
    float        m_totalAngle{ 0.f };
    float        m_travel{ 0.f };
    int          m_dir{ 1 };
};