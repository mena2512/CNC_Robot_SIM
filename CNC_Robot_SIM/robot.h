#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// ============================================================
//  Robot — the entire machine state in one place.
//  Commands read from it and write back to it each frame.
// ============================================================
class Robot
{
public:
    // --- Position ---
    sf::Vector2f pos{ 0.f, 0.f };   // current tool position (world units)

    // --- Feed ---
    float        feed{ 100.f };       // current feed rate (units/sec), set by F word

    // --- Path geometry (owned here, rendered externally) ---
    // Uses sf::PrimitiveType::Lines — every two vertices = one isolated segment.
    // G00 never pushes vertices; G01/G02/G03 always push pairs.
    std::vector<sf::Vertex> path;

    // --- Stats (read-only for HUD) ---
    int  commandsExecuted{ 0 };
    float totalDistance{ 0.f };

    // Append one cutting segment (start → end) to the path buffer.
    void addSegment(sf::Vector2f from, sf::Vector2f to, sf::Color color = sf::Color::White)
    {
        path.push_back({ from, color });
        path.push_back({ to,   color });
        totalDistance += std::sqrt(
            (to.x - from.x) * (to.x - from.x) +
            (to.y - from.y) * (to.y - from.y));
    }

    void reset()
    {
        pos = { 0.f, 0.f };
        feed = 100.f;
        path.clear();
        commandsExecuted = 0;
        totalDistance = 0.f;
    }
};