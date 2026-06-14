#pragma once
#include "Command.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// G02 (CW) and G03 (CCW) circular interpolation.
// All arc state is encapsulated — no external flags needed.
class ArcCommand : public Command
{
public:
    ArcCommand(sf::Vector2f target, sf::Vector2f centerOffset, int type, float feedWord = -1.f)
        : m_target(target), m_centerOffset(centerOffset), m_type(type), m_feedWord(feedWord) {
    }

    void execute(Robot& robot, float dt) override
    {
        if (m_finished) return;

        if (m_feedWord > 0.f)
            robot.feed = m_feedWord;

        if (!m_init)
        {
            m_center = robot.pos + m_centerOffset;
            m_radius = vecLen(m_center, robot.pos);
            if (m_radius < 1e-4f) m_radius = 1e-4f;

            sf::Vector2f sv = robot.pos - m_center;
            sf::Vector2f ev = m_target - m_center;

            m_startAngle = normAngle(std::atan2(sv.y, sv.x));
            float endAng = normAngle(std::atan2(ev.y, ev.x));

            m_dir = (m_type == 2) ? -1 : 1;
            m_totalAngle = (m_dir == 1)
                ? normAngle(endAng - m_startAngle)
                : normAngle(m_startAngle - endAng);

            if (m_totalAngle < 1e-4f) m_totalAngle = 6.2831853f;

            m_travel = 0.f;
            m_prevPos = robot.pos;
            m_init = true;
        }

        float dA = (robot.feed / m_radius) * dt;
        m_travel += dA;

        if (m_travel >= m_totalAngle)
        {
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

    bool        isFinished() const override { return m_finished; }
    std::string label()      const override
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << (m_type == 2 ? "G02" : "G03")
            << "  X" << m_target.x << "  Y" << m_target.y
            << "  I" << m_centerOffset.x << "  J" << m_centerOffset.y;
        if (m_feedWord > 0.f) ss << "  F" << m_feedWord;
        return ss.str();
    }

private:
    sf::Vector2f m_target, m_centerOffset, m_center, m_prevPos;
    int          m_type;
    float        m_feedWord;
    bool         m_init{ false };
    bool         m_finished{ false };
    float        m_radius{ 1.f };
    float        m_startAngle{ 0.f };
    float        m_totalAngle{ 0.f };
    float        m_travel{ 0.f };
    int          m_dir{ 1 };
};