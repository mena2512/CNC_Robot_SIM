#include "MoveCommand.h"
#include <cmath>

MoveCommand::MoveCommand(float x, float y) {
    targetX = x;
    targetY = y;
}

bool MoveCommand::isFinished() {
    return finished;
}

void MoveCommand::execute(Robot& robot, float dt) {
    float dx = targetX - robot.x;
    float dy = targetY - robot.y;

    float dist = std::sqrt(dx * dx + dy * dy);

    // ✅ stop condition
    if (dist < 1.5f) {
        robot.x = targetX;
        robot.y = targetY;
        finished = true;
        return;
    }

    // ✅ normalize direction
    float dirX = dx / dist;
    float dirY = dy / dist;

    // ✅ smooth movement using dt
    robot.x += dirX * speed * dt;
    robot.y += dirY * speed * dt;
}