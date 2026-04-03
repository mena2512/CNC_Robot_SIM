#pragma once
#include "Command.h"

class MoveCommand : public Command {
private:
    float targetX, targetY;
    float speed = 120.0f;
    bool finished = false;

public:
    MoveCommand(float x, float y);

    void execute(Robot& robot, float dt) override;
    bool isFinished() override;
};