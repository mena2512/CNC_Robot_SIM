#pragma once
#include "Robot.h"

// ============================================================
//  Command — abstract base for every G-code motion.
//
//  Contract:
//    execute(robot, dt) — advance the command by dt seconds.
//                         May call robot.addSegment() for cutting moves.
//    isFinished()       — returns true once the target has been reached.
//
//  Ownership: held as unique_ptr<Command> in CommandQueue.
//  No raw new/delete anywhere.
// ============================================================
class Command
{
public:
    virtual void execute(Robot& robot, float dt) = 0;
    virtual bool isFinished() const = 0;
    virtual ~Command() = default;

    // Human-readable label for the HUD (e.g. "G01 X120.00 Y-45.00")
    virtual std::string label() const = 0;

protected:
    // Shared math helpers available to all subclasses

    static float vecLen(sf::Vector2f a, sf::Vector2f b)
    {
        auto d = b - a;
        return std::sqrt(d.x * d.x + d.y * d.y);
    }

    static sf::Vector2f norm(sf::Vector2f v)
    {
        float l = std::sqrt(v.x * v.x + v.y * v.y);
        return (l > 1e-6f) ? v / l : sf::Vector2f{ 0.f, 0.f };
    }

    static float normAngle(float a)
    {
        const float TP = 6.2831853f;
        while (a < 0)  a += TP;
        while (a >= TP)  a -= TP;
        return a;
    }
};