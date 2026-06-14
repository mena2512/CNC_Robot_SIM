#pragma once
#include "Command.h"
#include <sstream>
#include <iomanip>

// G00 — rapid move, no geometry.
// G01 — cutting move, calls robot.addSegment() each frame.
class LinearCommand : public Command
{
public:
    LinearCommand(sf::Vector2f target, int type, float feedWord = -1.f)
        : m_target(target), m_type(type), m_feedWord(feedWord) {
    }

    void execute(Robot& robot, float dt) override
    {
        if (m_finished) return;

        if (m_feedWord > 0.f)
            robot.feed = m_feedWord;

        float speed = (m_type == 0) ? RAPID_SPEED : robot.feed;
        float step = speed * dt;

        sf::Vector2f prev = robot.pos;
        float dist = vecLen(robot.pos, m_target);

        if (dist <= step || dist < 1e-5f)
        {
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

    bool        isFinished() const override { return m_finished; }
    std::string label()      const override
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << (m_type == 0 ? "G00" : "G01")
            << "  X" << m_target.x << "  Y" << m_target.y;
        if (m_feedWord > 0.f) ss << "  F" << m_feedWord;
        return ss.str();
    }

private:
    static constexpr float RAPID_SPEED = 500.f;
    sf::Vector2f m_target;
    int          m_type;
    float        m_feedWord;
    bool         m_finished{ false };
};