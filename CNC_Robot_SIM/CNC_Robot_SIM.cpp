#include <SFML/Graphics.hpp>
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "Robot.h"
#include "CommandQueue.h"

// ============================================================
//  main.cpp — CNC Simulator
//
//  Architecture:
//    Robot        — machine state (pos, feed, path buffer)
//    CommandQueue — owns all programs, drives execution
//    LinearCommand / ArcCommand / DwellCommand — motion logic
//
//  Controls:
//    SPACE        — play / pause
//    R            — reset current program
//    HOME         — re-centre view
//    Up / Down    — feed override ±
//    Scroll       — zoom
//    Middle drag  — pan
//    PROG+ / PROG- buttons — cycle programs
// ============================================================

// ================= FILE DIALOG =================
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

// ================= GRID =================
static void drawGrid(sf::RenderWindow& win, const sf::View& view)
{
    const float MAJOR = 100.f;
    const float MINOR = 25.f;

    auto tl = view.getCenter() - view.getSize() / 2.f;
    auto br = view.getCenter() + view.getSize() / 2.f;

    std::vector<sf::Vertex> verts;
    verts.reserve(1024);

    auto push = [&](sf::Vector2f a, sf::Vector2f b, sf::Color c)
        {
            verts.push_back({ a, c });
            verts.push_back({ b, c });
        };

    // Minor lines
    for (int x = (int)(tl.x / MINOR) - 1; x <= (int)(br.x / MINOR) + 1; ++x)
    {
        float xp = x * MINOR;
        if (std::fmod(std::abs(xp) + 0.01f, MAJOR) < 1.f) continue;
        push({ xp, tl.y }, { xp, br.y }, sf::Color(45, 45, 45));
    }
    for (int y = (int)(tl.y / MINOR) - 1; y <= (int)(br.y / MINOR) + 1; ++y)
    {
        float yp = y * MINOR;
        if (std::fmod(std::abs(yp) + 0.01f, MAJOR) < 1.f) continue;
        push({ tl.x, yp }, { br.x, yp }, sf::Color(45, 45, 45));
    }

    // Major lines
    for (int x = (int)(tl.x / MAJOR) - 1; x <= (int)(br.x / MAJOR) + 1; ++x)
    {
        float xp = x * MAJOR;
        sf::Color c = (x == 0) ? sf::Color(180, 60, 60) : sf::Color(80, 80, 80);
        push({ xp, tl.y }, { xp, br.y }, c);
    }
    for (int y = (int)(tl.y / MAJOR) - 1; y <= (int)(br.y / MAJOR) + 1; ++y)
    {
        float yp = y * MAJOR;
        sf::Color c = (y == 0) ? sf::Color(60, 180, 60) : sf::Color(80, 80, 80);
        push({ tl.x, yp }, { br.x, yp }, c);
    }

    win.draw(verts.data(), verts.size(), sf::PrimitiveType::Lines);
}

// ================= BUTTON =================
struct Button
{
    sf::RectangleShape box;
    sf::Text           label;

    Button(const sf::Font& font, const std::string& text, sf::Vector2f pos,
        sf::Vector2f size = { 95.f, 30.f })
        : label(font)
    {
        box.setSize(size);
        box.setPosition(pos);
        box.setFillColor(sf::Color(60, 60, 60));
        box.setOutlineThickness(1.f);
        box.setOutlineColor(sf::Color(110, 110, 110));

        label.setString(text);
        label.setCharacterSize(13);
        label.setFillColor(sf::Color(220, 220, 220));
        label.setPosition(pos + sf::Vector2f(10.f, 8.f));
    }

    bool contains(sf::Vector2f m) const { return box.getGlobalBounds().contains(m); }

    void setHovered(bool h)
    {
        box.setFillColor(h ? sf::Color(90, 90, 90) : sf::Color(60, 60, 60));
    }

    void draw(sf::RenderWindow& w) { w.draw(box); w.draw(label); }
};

// ================= HELPERS =================
static std::string fmt(float v, int dec = 2)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(dec) << v;
    return ss.str();
}

// ================= MAIN =================
int main()
{
    const unsigned W = 1200, H = 760;
    sf::RenderWindow window(sf::VideoMode({ W, H }), "CNC Simulator");
    window.setFramerateLimit(60);

    sf::View worldView(sf::FloatRect({ 0.f, 0.f }, { (float)W, (float)H }));
    worldView.setCenter({ 0.f, 0.f });

    sf::Font font;
    font.openFromFile("arial.ttf");

    // ---- Core objects ----
    Robot        robot;
    CommandQueue queue;
    queue.loadFile("command.txt");

    // ---- Simulator state ----
    bool  running = false;
    float overrideF = 1.f;    // feed rate multiplier
    float zoomLevel = 1.f;

    // ---- Navigation ----
    bool         dragging = false;
    sf::Vector2i lastMouse;

    // ---- Tool marker ----
    sf::CircleShape tool(5.f);
    tool.setOrigin({ 5.f, 5.f });
    tool.setFillColor(sf::Color::Red);
    tool.setOutlineThickness(1.5f);
    tool.setOutlineColor(sf::Color(255, 130, 130));

    // ---- Buttons ----
    const float BY = (float)H - 44.f;
    Button btnPlay(font, "PLAY", { 10,  BY });
    Button btnPause(font, "PAUSE", { 115, BY });
    Button btnReset(font, "RESET", { 220, BY });
    Button btnLoad(font, "LOAD", { 325, BY });
    Button btnProgP(font, "PROG +", { 450, BY });
    Button btnProgM(font, "PROG -", { 555, BY });

    // ---- HUD text ----
    sf::Text hud(font);
    hud.setCharacterSize(14);
    hud.setFillColor(sf::Color::White);
    hud.setPosition({ 12.f, 10.f });

    // ---- Command preview text ----
    sf::Text preview(font);
    preview.setCharacterSize(12);
    preview.setFillColor(sf::Color(180, 180, 180));
    preview.setPosition({ 12.f, 180.f });

    // ---- Timing ----
    sf::Clock clock;

    // ================= MAIN LOOP =================
    while (window.isOpen())
    {
        float dt = std::min(clock.restart().asSeconds(), 0.05f);

        // Apply feed override to robot before tick
        // (CommandQueue will use robot.feed which we scale here)
        // We store the "raw" feed in robot and scale externally
        // by temporarily multiplying — simplest approach.
        float savedFeed = robot.feed;
        robot.feed *= overrideF;

        sf::Vector2f mouseWin = sf::Vector2f(sf::Mouse::getPosition(window));

        // --- Update button hover ---
        for (auto* b : { &btnPlay, &btnPause, &btnReset,
                          &btnLoad, &btnProgP, &btnProgM })
            b->setHovered(b->contains(mouseWin));

        // --- Events ---
        while (auto ev = window.pollEvent())
        {
            if (ev->is<sf::Event::Closed>()) window.close();

            if (auto* sc = ev->getIf<sf::Event::MouseWheelScrolled>())
            {
                float f = (sc->delta > 0) ? 0.9f : 1.1f;
                worldView.zoom(f);
                zoomLevel *= f;
            }

            if (auto* p = ev->getIf<sf::Event::MouseButtonPressed>())
            {
                if (p->button == sf::Mouse::Button::Middle)
                {
                    dragging = true;
                    lastMouse = sf::Mouse::getPosition(window);
                }

                if (p->button == sf::Mouse::Button::Left)
                {
                    if (btnPlay.contains(mouseWin))  running = true;
                    if (btnPause.contains(mouseWin)) running = false;

                    if (btnReset.contains(mouseWin))
                    {
                        running = false;
                        queue.reset(robot);
                    }

                    if (btnLoad.contains(mouseWin))
                    {
                        auto path = openFileDialog();
                        if (!path.empty())
                        {
                            queue.loadFile(path);
                            queue.reset(robot);
                            running = false;
                        }
                    }

                    if (btnProgP.contains(mouseWin)) queue.nextProgram(robot);
                    if (btnProgM.contains(mouseWin)) queue.prevProgram(robot);
                }
            }

            if (auto* r = ev->getIf<sf::Event::MouseButtonReleased>())
                if (r->button == sf::Mouse::Button::Middle)
                    dragging = false;

            if (auto* k = ev->getIf<sf::Event::KeyPressed>())
            {
                if (k->code == sf::Keyboard::Key::Space)
                    running = !running;

                if (k->code == sf::Keyboard::Key::R)
                {
                    running = false; queue.reset(robot);
                }

                if (k->code == sf::Keyboard::Key::Home)
                {
                    worldView.setCenter({ 0.f, 0.f });
                    worldView.setSize({ (float)W, (float)H });
                    zoomLevel = 1.f;
                }
            }
        }

        // --- Pan ---
        if (dragging)
        {
            sf::Vector2i m = sf::Mouse::getPosition(window);
            worldView.move(sf::Vector2f(lastMouse - m) * zoomLevel);
            lastMouse = m;
        }

        // --- Feed override ---
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            overrideF = std::min(overrideF + 0.4f * dt, 5.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            overrideF = std::max(overrideF - 0.4f * dt, 0.1f);

        // --- Tick ---
        if (running && !queue.empty())
            queue.tick(robot, dt);

        // Restore real feed (override is transient, not stored)
        robot.feed = savedFeed;

        // Sync feed from last executed command
        // (robot.feed may have been updated inside a command via F word)

        tool.setPosition(robot.pos);

        // ---- Build HUD string ----
        std::string status = running
            ? (queue.isFinished() ? "COMPLETE" : "RUNNING")
            : "PAUSED";

        hud.setString(
            "X: " + fmt(robot.pos.x) + "   Y: " + fmt(robot.pos.y) +
            "\nFeed:  " + fmt(robot.feed) + " u/s  x" + fmt(overrideF) +
            "\nCmd:   " + std::to_string(queue.commandIndex()) +
            " / " + std::to_string(queue.commandCount()) +
            "\nProg:  " + std::to_string(queue.programIndex() + 1) +
            " / " + std::to_string(queue.programCount()) +
            "\nDist:  " + fmt(robot.totalDistance) + " u" +
            "\nNow:   " + queue.currentLabel() +
            "\nState: " + status +
            "\n\n[SPACE] Play/Pause   [R] Reset   [HOME] Re-centre"
        );

        // ---- Command preview list ----
        auto lines = queue.preview(8);
        std::string prevStr = "-- Next --\n";
        for (auto& l : lines) prevStr += l + "\n";
        preview.setString(prevStr);

        // ================= RENDER =================
        window.clear(sf::Color(18, 18, 18));

        // World space
        window.setView(worldView);
        drawGrid(window, worldView);

        // Origin marker
        std::array<sf::Vertex, 4> cross = {
            sf::Vertex{{ -10.f,   0.f }, sf::Color(200,200,0)},
            sf::Vertex{{  10.f,   0.f }, sf::Color(200,200,0)},
            sf::Vertex{{   0.f, -10.f }, sf::Color(200,200,0)},
            sf::Vertex{{   0.f,  10.f }, sf::Color(200,200,0)},
        };
        window.draw(cross.data(), 4, sf::PrimitiveType::Lines);

        // Toolpath — Lines primitive, isolated pairs, no ghost G00 lines
        auto& path = robot.path;
        if (path.size() >= 2)
            window.draw(path.data(), path.size(), sf::PrimitiveType::Lines);

        window.draw(tool);

        // Screen space (UI)
        window.setView(window.getDefaultView());

        // HUD background
        sf::RectangleShape hudBg({ 340.f, 160.f });
        hudBg.setPosition({ 6.f, 6.f });
        hudBg.setFillColor(sf::Color(0, 0, 0, 160));
        hudBg.setOutlineThickness(1.f);
        hudBg.setOutlineColor(sf::Color(70, 70, 70));
        window.draw(hudBg);
        window.draw(hud);

        // Preview background
        sf::RectangleShape prevBg({ 300.f, 150.f });
        prevBg.setPosition({ 6.f, 174.f });
        prevBg.setFillColor(sf::Color(0, 0, 0, 140));
        prevBg.setOutlineThickness(1.f);
        prevBg.setOutlineColor(sf::Color(70, 70, 70));
        window.draw(prevBg);
        window.draw(preview);

        btnPlay.draw(window);
        btnPause.draw(window);
        btnReset.draw(window);
        btnLoad.draw(window);
        btnProgP.draw(window);
        btnProgM.draw(window);

        window.display();
    }

    return 0;
}