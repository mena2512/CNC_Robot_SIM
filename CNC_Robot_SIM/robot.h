#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>

// ============================================================
//  Robot — machine state + path storage.
//
//  Two-layer path system:
//
//  1. permanentPath  — all segments older than TRAIL_DURATION.
//                      Rendered at a fixed dim colour. Never changes.
//
//  2. trailSegments  — a rolling deque of recent segments, each
//                      carrying its age. Every frame their alpha is
//                      recomputed: newest = full white, oldest = dim.
//                      Once a segment ages past TRAIL_DURATION it is
//                      flushed into permanentPath and removed.
//
//  This way the fade is always smooth and costs only O(trail size)
//  per frame, regardless of how long the total path is.
// ============================================================

// Colour palette — single source of truth
namespace Palette
{
    // Background: very dark navy-charcoal
    inline constexpr sf::Color BG{ 15,  20,  30,  255 };

    // Permanent path: visible cool grey — bright enough to read clearly
    inline constexpr sf::Color PATH_DIM{ 100, 118, 145, 255 };

    // Trail tip: crisp near-white with a faint blue tint
    inline constexpr sf::Color TRAIL_BRIGHT{ 235, 242, 255, 255 };

    // Grid major / minor
    inline constexpr sf::Color GRID_MAJOR{ 38,  48,  62,  255 };
    inline constexpr sf::Color GRID_MINOR{ 25,  32,  42,  255 };

    // Axes
    inline constexpr sf::Color AXIS_X{ 160,  50,  50,  200 };
    inline constexpr sf::Color AXIS_Y{ 50, 140,  80,  200 };

    // Origin cross
    inline constexpr sf::Color ORIGIN{ 180, 180,  60,  255 };

    // Tool head
    inline constexpr sf::Color TOOL_FILL{ 220,  80,  60,  255 };
    inline constexpr sf::Color TOOL_RING{ 255, 160, 140,  180 };

    // UI panel background
    inline constexpr sf::Color PANEL_BG{ 10,  14,  22,  210 };
    inline constexpr sf::Color PANEL_BORDER{ 40,  55,  75,  255 };

    // Button states
    inline constexpr sf::Color BTN_NORMAL{ 28,  38,  55,  255 };
    inline constexpr sf::Color BTN_HOVER{ 45,  62,  88,  255 };
    inline constexpr sf::Color BTN_ACTIVE{ 60, 120, 180,  255 };
    inline constexpr sf::Color BTN_TEXT{ 190, 205, 225,  255 };

    // HUD text
    inline constexpr sf::Color HUD_PRIMARY{ 200, 215, 235,  255 };
    inline constexpr sf::Color HUD_DIM{ 100, 120, 150,  255 };
    inline constexpr sf::Color HUD_ACCENT{ 80, 160, 220,  255 };
}

struct TrailSegment
{
    sf::Vector2f from;
    sf::Vector2f to;
    float        age{ 0.f };   // seconds since this segment was cut
};

class Robot
{
public:
    // ---- Machine state ----
    sf::Vector2f pos{ 0.f, 0.f };
    float        feed{ 100.f };        // raw G-code feed, never override-scaled

    // ---- Stats ----
    int   commandsExecuted{ 0 };
    float totalDistance{ 0.f };

    // ---- Path storage ----
    // How long (seconds) a segment stays in the bright fading trail
    static constexpr float TRAIL_DURATION = 3.5f;

    std::vector<sf::Vertex> permanentPath; // dim, static, never rebuilt
    std::deque<TrailSegment> trail;        // recent segments, faded each frame

    // --------------------------------------------------------
    //  addSegment — called by cutting commands (G01/G02/G03).
    //  New segments always enter the trail at age=0.
    //  G00 never calls this.
    // --------------------------------------------------------
    void addSegment(sf::Vector2f from, sf::Vector2f to)
    {
        trail.push_back({ from, to, 0.f });

        float d = std::sqrt((to.x - from.x) * (to.x - from.x) +
            (to.y - from.y) * (to.y - from.y));
        totalDistance += d;
    }

    // --------------------------------------------------------
    //  updateTrail — call once per frame with dt.
    //  Ages every trail segment, flushes expired ones to
    //  permanentPath, and rebuilds the trail vertex buffer.
    // --------------------------------------------------------
    void updateTrail(float dt, std::vector<sf::Vertex>& trailVerts)
    {
        // Age all segments
        for (auto& s : trail)
            s.age += dt;

        // Flush expired segments to permanentPath
        while (!trail.empty() && trail.front().age >= TRAIL_DURATION)
        {
            auto& s = trail.front();
            permanentPath.push_back({ s.from, Palette::PATH_DIM });
            permanentPath.push_back({ s.to,   Palette::PATH_DIM });
            trail.pop_front();
        }

        // Rebuild trail vertex buffer with per-segment alpha
        trailVerts.clear();
        trailVerts.reserve(trail.size() * 2);

        for (auto& s : trail)
        {
            // t=0 → newest (bright),  t=1 → oldest (fading out)
            float t = s.age / TRAIL_DURATION;

            // Ease-in curve: stays bright longer then drops off fast
            // t_curved = t^2   keeps the tip white for ~half the duration
            float tc = t * t;

            auto lerp8 = [](uint8_t a, uint8_t b, float tt) -> uint8_t {
                return (uint8_t)(a + (b - a) * tt);
                };

            sf::Color c{
                lerp8(Palette::TRAIL_BRIGHT.r, Palette::PATH_DIM.r, tc),
                lerp8(Palette::TRAIL_BRIGHT.g, Palette::PATH_DIM.g, tc),
                lerp8(Palette::TRAIL_BRIGHT.b, Palette::PATH_DIM.b, tc),
                lerp8(255u, Palette::PATH_DIM.a, tc)
            };

            trailVerts.push_back({ s.from, c });
            trailVerts.push_back({ s.to,   c });
        }
    }

    void reset()
    {
        pos = { 0.f, 0.f };
        feed = 100.f;
        permanentPath.clear();
        trail.clear();
        commandsExecuted = 0;
        totalDistance = 0.f;
    }
};