#include <SFML/Graphics.hpp>
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <array>
#include <vector>
#include <optional>
#include <cmath>

#include "Robot.h"
#include "CommandQueue.h"

// ============================================================
//  CNC Simulator — main.cpp
//
//  Visual design: dark navy-charcoal, professional CAM aesthetic.
//  Toolpath uses a two-layer system:
//    - permanentPath  : dim static grey  (everything older than trail)
//    - trailVerts     : smooth fade from bright-white → dim grey
//  The fading trail is recomputed every frame from Robot::trail.
//  Feed override controls playback speed only, never visual output.
// ============================================================

// ---------------------------------------------------------------
//  File dialog
// ---------------------------------------------------------------
static std::string openFileDialog()
{
    char buf[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "G-Code Files\0*.txt;*.nc;*.gcode\0All Files\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
    return GetOpenFileNameA(&ofn) ? std::string(buf) : "";
}

// ---------------------------------------------------------------
//  Grid
// ---------------------------------------------------------------
static void drawGrid(sf::RenderWindow& win, const sf::View& view)
{
    const float MAJOR = 100.f;
    const float MINOR = 25.f;

    auto tl = view.getCenter() - view.getSize() / 2.f;
    auto br = view.getCenter() + view.getSize() / 2.f;

    std::vector<sf::Vertex> v;
    v.reserve(2048);

    auto push = [&](sf::Vector2f a, sf::Vector2f b, sf::Color c)
        { v.push_back({ a,c }); v.push_back({ b,c }); };

    for (int x = (int)(tl.x / MINOR) - 1; x <= (int)(br.x / MINOR) + 1; ++x)
    {
        float xp = x * MINOR;
        bool isMajor = std::fmod(std::abs(xp) + 0.5f, MAJOR) < 1.f;
        if (isMajor) continue;
        push({ xp,tl.y }, { xp,br.y }, Palette::GRID_MINOR);
    }
    for (int y = (int)(tl.y / MINOR) - 1; y <= (int)(br.y / MINOR) + 1; ++y)
    {
        float yp = y * MINOR;
        bool isMajor = std::fmod(std::abs(yp) + 0.5f, MAJOR) < 1.f;
        if (isMajor) continue;
        push({ tl.x,yp }, { br.x,yp }, Palette::GRID_MINOR);
    }

    for (int x = (int)(tl.x / MAJOR) - 1; x <= (int)(br.x / MAJOR) + 1; ++x)
    {
        float xp = x * MAJOR;
        push({ xp,tl.y }, { xp,br.y }, x == 0 ? Palette::AXIS_X : Palette::GRID_MAJOR);
    }
    for (int y = (int)(tl.y / MAJOR) - 1; y <= (int)(br.y / MAJOR) + 1; ++y)
    {
        float yp = y * MAJOR;
        push({ tl.x,yp }, { br.x,yp }, y == 0 ? Palette::AXIS_Y : Palette::GRID_MAJOR);
    }

    win.draw(v.data(), v.size(), sf::PrimitiveType::Lines);
}

// ---------------------------------------------------------------
//  Button — clean flat style matching the palette.
//  sf::Text is held in std::optional to avoid default-constructing
//  it (SFML 3 requires a font at construction time).
// ---------------------------------------------------------------
struct Button
{
    sf::RectangleShape      box;
    std::optional<sf::Text> label;
    bool                    active{ false };

    Button() = default;

    void init(const sf::Font& font, const std::string& text,
        sf::Vector2f pos, sf::Vector2f size = { 88.f, 32.f })
    {
        box.setSize(size);
        box.setPosition(pos);
        box.setFillColor(Palette::BTN_NORMAL);
        box.setOutlineThickness(1.f);
        box.setOutlineColor(Palette::PANEL_BORDER);

        label.emplace(font);
        label->setString(text);
        label->setCharacterSize(12);
        label->setFillColor(Palette::BTN_TEXT);
        label->setLetterSpacing(1.3f);

        auto b = label->getLocalBounds();
        label->setOrigin({ b.position.x + b.size.x * 0.5f,
                           b.position.y + b.size.y * 0.5f });
        label->setPosition(pos + size / 2.f);
    }

    bool contains(sf::Vector2f m) const { return box.getGlobalBounds().contains(m); }

    void setHovered(bool h)
    {
        if (active)
            box.setFillColor(Palette::BTN_ACTIVE);
        else
            box.setFillColor(h ? Palette::BTN_HOVER : Palette::BTN_NORMAL);
    }

    void draw(sf::RenderWindow& w) { w.draw(box); if (label) w.draw(*label); }
};

// ---------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------
static std::string fmt(float v, int dec = 2)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(dec) << v;
    return ss.str();
}

// Draw a single-pixel horizontal separator line in screen space
static void drawSeparator(sf::RenderWindow& win, float x, float y, float w)
{
    std::array<sf::Vertex, 2> line = {
        sf::Vertex{{x,   y}, Palette::PANEL_BORDER},
        sf::Vertex{{x + w, y}, Palette::PANEL_BORDER}
    };
    win.draw(line.data(), 2, sf::PrimitiveType::Lines);
}

// ---------------------------------------------------------------
//  Sidebar panel — left side, fixed width
// ---------------------------------------------------------------
struct Sidebar
{
    static constexpr float W = 240.f;
    static constexpr float PAD = 16.f;

    sf::RectangleShape      panel;
    std::optional<sf::Text> txtCoords, txtFeed, txtStatus, txtPreview, txtHelp;

    Button btnPlay, btnPause, btnReset, btnLoad, btnProgP, btnProgM;

    void init(const sf::Font& font, float winH)
    {
        panel.setSize({ W, winH });
        panel.setPosition({ 0.f, 0.f });
        panel.setFillColor(Palette::PANEL_BG);
        panel.setOutlineThickness(0.f);

        auto makeText = [&](std::optional<sf::Text>& t, unsigned size, sf::Color col, float y)
            {
                t.emplace(font);
                t->setCharacterSize(size);
                t->setFillColor(col);
                t->setPosition({ PAD, y });
            };

        makeText(txtCoords, 13, Palette::HUD_PRIMARY, 16.f);
        makeText(txtFeed, 12, Palette::HUD_DIM, 76.f);
        makeText(txtStatus, 11, Palette::HUD_ACCENT, 240.f);
        makeText(txtPreview, 11, Palette::HUD_DIM, 272.f);
        makeText(txtHelp, 10, Palette::HUD_DIM, winH - 130.f);

        // Buttons — two rows of three
        float bw = (W - PAD * 2 - 8.f) / 3.f;
        float bh = 30.f;
        float row1 = winH - 86.f;
        float row2 = winH - 48.f;

        btnPlay.init(font, "PLAY", { PAD + 0 * (bw + 4), row1 }, { bw, bh });
        btnPause.init(font, "PAUSE", { PAD + 1 * (bw + 4), row1 }, { bw, bh });
        btnReset.init(font, "RESET", { PAD + 2 * (bw + 4), row1 }, { bw, bh });
        btnLoad.init(font, "LOAD", { PAD + 0 * (bw + 4), row2 }, { bw, bh });
        btnProgP.init(font, "PROG +", { PAD + 1 * (bw + 4), row2 }, { bw, bh });
        btnProgM.init(font, "PROG -", { PAD + 2 * (bw + 4), row2 }, { bw, bh });
    }

    void update(const Robot& robot, const CommandQueue& queue,
        float overrideF, bool running, sf::Vector2f mouse)
    {
        bool done = queue.isFinished();

        txtCoords->setString(
            "X  " + fmt(robot.pos.x, 3) + "\n"
            "Y  " + fmt(robot.pos.y, 3)
        );

        txtFeed->setString(
            "FEED    " + fmt(robot.feed, 0) + " u/s\n"
            "ACTUAL  " + fmt(robot.feed * overrideF, 0) + " u/s\n"
            "OVRD    " + fmt(overrideF * 100.f, 0) + " %\n\n"
            "PROG    " + std::to_string(queue.programIndex() + 1) +
            " / " + std::to_string(queue.programCount()) + "\n"
            "CMD     " + std::to_string(queue.commandIndex()) +
            " / " + std::to_string(queue.commandCount()) + "\n"
            "DIST    " + fmt(robot.totalDistance, 1) + " u"
        );

        std::string st = done ? "COMPLETE" :
            running ? "RUNNING" : "PAUSED";
        txtStatus->setString(st);
        txtStatus->setFillColor(
            done ? sf::Color(80, 200, 120, 255) :
            running ? Palette::HUD_ACCENT :
            sf::Color(180, 140, 60, 255)
        );

        auto lines = queue.preview(7);
        std::string prev = "QUEUE\n";
        for (size_t i = 0; i < lines.size(); ++i)
        {
            prev += (i == 0 ? "> " : "  ");
            std::string lbl = lines[i];
            if (lbl.size() > 26) lbl = lbl.substr(0, 24) + "..";
            prev += lbl + "\n";
        }
        txtPreview->setString(prev);

        txtHelp->setString(
            "SPACE  play / pause\n"
            "R      reset\n"
            "HOME   re-centre\n"
            "UP/DN  feed override\n"
            "SCROLL zoom\n"
            "WHEEL  pan"
        );

        // Button active states
        btnPlay.active = running && !done;
        btnPause.active = !running;

        // Hover
        btnPlay.setHovered(btnPlay.contains(mouse));
        btnPause.setHovered(btnPause.contains(mouse));
        btnReset.setHovered(btnReset.contains(mouse));
        btnLoad.setHovered(btnLoad.contains(mouse));
        btnProgP.setHovered(btnProgP.contains(mouse));
        btnProgM.setHovered(btnProgM.contains(mouse));
    }

    void draw(sf::RenderWindow& win)
    {
        win.draw(panel);

        // Right border
        float h = panel.getSize().y;
        std::array<sf::Vertex, 2> border = {
            sf::Vertex{{W, 0.f}, Palette::PANEL_BORDER},
            sf::Vertex{{W, h  }, Palette::PANEL_BORDER}
        };
        win.draw(border.data(), 2, sf::PrimitiveType::Lines);

        // Separators
        drawSeparator(win, 8.f, 68.f, W - 16.f);   // below coords
        drawSeparator(win, 8.f, 232.f, W - 16.f);   // below feed block
        drawSeparator(win, 8.f, 264.f, W - 16.f);   // below status

        win.draw(*txtCoords);
        win.draw(*txtFeed);
        win.draw(*txtStatus);
        win.draw(*txtPreview);
        win.draw(*txtHelp);

        btnPlay.draw(win);
        btnPause.draw(win);
        btnReset.draw(win);
        btnLoad.draw(win);
        btnProgP.draw(win);
        btnProgM.draw(win);
    }

    // Convenience checks for main event loop
    bool clickPlay(sf::Vector2f m) { return btnPlay.contains(m); }
    bool clickPause(sf::Vector2f m) { return btnPause.contains(m); }
    bool clickReset(sf::Vector2f m) { return btnReset.contains(m); }
    bool clickLoad(sf::Vector2f m) { return btnLoad.contains(m); }
    bool clickProgP(sf::Vector2f m) { return btnProgP.contains(m); }
    bool clickProgM(sf::Vector2f m) { return btnProgM.contains(m); }
};

// ---------------------------------------------------------------
//  Tool head — small cross + circle, looks more CAM-like
// ---------------------------------------------------------------
static void drawTool(sf::RenderWindow& win, sf::Vector2f pos)
{
    const float R = 5.f;
    const float ARM = 9.f;

    sf::CircleShape circle(R);
    circle.setOrigin({ R, R });
    circle.setPosition(pos);
    circle.setFillColor(sf::Color::Transparent);
    circle.setOutlineThickness(1.5f);
    circle.setOutlineColor(Palette::TOOL_FILL);
    win.draw(circle);

    std::array<sf::Vertex, 4> cross = {
        sf::Vertex{pos + sf::Vector2f{-ARM, 0.f}, Palette::TOOL_FILL},
        sf::Vertex{pos + sf::Vector2f{ ARM, 0.f}, Palette::TOOL_FILL},
        sf::Vertex{pos + sf::Vector2f{0.f, -ARM}, Palette::TOOL_FILL},
        sf::Vertex{pos + sf::Vector2f{0.f,  ARM}, Palette::TOOL_FILL},
    };
    win.draw(cross.data(), 4, sf::PrimitiveType::Lines);
}

// ---------------------------------------------------------------
//  Origin marker
// ---------------------------------------------------------------
static void drawOrigin(sf::RenderWindow& win)
{
    const float S = 7.f;
    std::array<sf::Vertex, 4> v = {
        sf::Vertex{{-S, 0.f}, Palette::ORIGIN},
        sf::Vertex{{ S, 0.f}, Palette::ORIGIN},
        sf::Vertex{{0.f,-S }, Palette::ORIGIN},
        sf::Vertex{{0.f, S }, Palette::ORIGIN},
    };
    win.draw(v.data(), 4, sf::PrimitiveType::Lines);
}

// ---------------------------------------------------------------
//  MAIN
// ---------------------------------------------------------------
int main()
{
    const unsigned W = 1280, H = 800;

    sf::RenderWindow window(sf::VideoMode({ W, H }), "CNC Simulator",
        sf::Style::Default);
    window.setFramerateLimit(60);

    // World view — offset so sidebar doesn't overlap the canvas
    const float SW = Sidebar::W; // sidebar width
    sf::View worldView;
    {
        float vw = (float)W - SW;
        float vh = (float)H;
        worldView.setSize({ vw, vh });
        worldView.setCenter({ 0.f, 0.f });
        worldView.setViewport(sf::FloatRect(
            { SW / W, 0.f },
            { vw / W, 1.f }
        ));
    }

    sf::Font font;
    font.openFromFile("arial.ttf");

    // Core objects
    Robot        robot;
    CommandQueue queue;
    queue.loadFile("command.txt");

    // Sidebar
    Sidebar sidebar;
    sidebar.init(font, (float)H);

    // Trail vertex buffer (rebuilt every frame)
    std::vector<sf::Vertex> trailVerts;

    // Simulator state
    bool  running = false;
    float overrideF = 1.f;
    float zoomLevel = 1.f;

    // Pan
    bool         dragging = false;
    sf::Vector2i lastMouse;

    sf::Clock clock;

    // ============================================================
    //  MAIN LOOP
    // ============================================================
    while (window.isOpen())
    {
        float dt = std::min(clock.restart().asSeconds(), 0.05f);

        sf::Vector2f mouseWin = sf::Vector2f(sf::Mouse::getPosition(window));

        // ---- Events ----
        while (auto ev = window.pollEvent())
        {
            if (ev->is<sf::Event::Closed>()) window.close();

            if (auto* sc = ev->getIf<sf::Event::MouseWheelScrolled>())
            {
                // Only zoom if mouse is over the canvas, not the sidebar
                if (mouseWin.x > SW)
                {
                    float f = (sc->delta > 0) ? 0.88f : 1.14f;
                    worldView.zoom(f);
                    zoomLevel *= f;
                }
            }

            if (auto* p = ev->getIf<sf::Event::MouseButtonPressed>())
            {
                if (p->button == sf::Mouse::Button::Middle && mouseWin.x > SW)
                {
                    dragging = true;
                    lastMouse = sf::Mouse::getPosition(window);
                }

                if (p->button == sf::Mouse::Button::Left)
                {
                    if (sidebar.clickPlay(mouseWin)) running = true;
                    if (sidebar.clickPause(mouseWin)) running = false;

                    if (sidebar.clickReset(mouseWin))
                    {
                        running = false; queue.reset(robot);
                    }

                    if (sidebar.clickLoad(mouseWin))
                    {
                        auto path = openFileDialog();
                        if (!path.empty())
                        {
                            queue.loadFile(path); queue.reset(robot); running = false;
                        }
                    }

                    if (sidebar.clickProgP(mouseWin)) queue.nextProgram(robot);
                    if (sidebar.clickProgM(mouseWin)) queue.prevProgram(robot);
                }
            }

            if (auto* r = ev->getIf<sf::Event::MouseButtonReleased>())
                if (r->button == sf::Mouse::Button::Middle) dragging = false;

            if (auto* k = ev->getIf<sf::Event::KeyPressed>())
            {
                using K = sf::Keyboard::Key;
                if (k->code == K::Space) running = !running;
                if (k->code == K::R) { running = false; queue.reset(robot); }
                if (k->code == K::Home)
                {
                    float vw = (float)W - SW, vh = (float)H;
                    worldView.setSize({ vw, vh });
                    worldView.setCenter({ 0.f, 0.f });
                    zoomLevel = 1.f;
                }
            }
        }

        // ---- Pan ----
        if (dragging)
        {
            sf::Vector2i m = sf::Mouse::getPosition(window);
            worldView.move(sf::Vector2f(lastMouse - m) * zoomLevel);
            lastMouse = m;
        }

        // ---- Feed override (Up/Down arrows) ----
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            overrideF = std::min(overrideF + 0.5f * dt, 5.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            overrideF = std::max(overrideF - 0.5f * dt, 0.1f);

        // ---- Tick — override scales motion speed only ----
        if (running && !queue.empty())
        {
            float rawFeed = robot.feed;
            robot.feed *= overrideF;

            queue.tick(robot, dt);

            // Recover raw feed (F-word updates inside tick are preserved via divide)
            robot.feed /= overrideF;
        }

        // ---- Update trail (age segments, flush expired ones) ----
        robot.updateTrail(dt, trailVerts);

        // ---- Update sidebar ----
        sidebar.update(robot, queue, overrideF, running, mouseWin);

        // ============================================================
        //  RENDER
        // ============================================================
        window.clear(Palette::BG);

        // --- World space (canvas) ---
        window.setView(worldView);
        drawGrid(window, worldView);
        drawOrigin(window);

        // Permanent path (dim, static)
        if (!robot.permanentPath.empty())
            window.draw(robot.permanentPath.data(),
                robot.permanentPath.size(),
                sf::PrimitiveType::Lines);

        // Fading trail (bright → dim, rebuilt each frame)
        if (!trailVerts.empty())
            window.draw(trailVerts.data(),
                trailVerts.size(),
                sf::PrimitiveType::Lines);

        drawTool(window, robot.pos);

        // --- Screen space (UI) ---
        window.setView(window.getDefaultView());
        sidebar.draw(window);

        window.display();
    }

    return 0;
}