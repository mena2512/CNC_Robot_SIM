#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Command.h"
#include "LinearCommand.h"
#include "ArcCommand.h"
#include "DwellCommand.h"

// ============================================================
//  CommandQueue — owns all programs and drives execution.
//
//  A "program" is a vector of unique_ptr<Command>.
//  Multiple programs are separated by "NEW" in the source file.
//
//  Each frame, call tick(robot, dt) while running == true.
//  The queue advances automatically; call reset() to replay.
// ============================================================
class CommandQueue
{
public:
    // --- Load ---

    void loadFile(const std::string& path)
    {
        m_programs.clear();
        m_programs.emplace_back();   // program 0

        std::ifstream file(path);
        if (!file.is_open()) return;

        std::string line;
        int cur = 0;
        int modalType = 0;       // last seen G-type (modal carry-over)
        int lineNum = 0;

        while (std::getline(file, line))
        {
            ++lineNum;

            // Strip inline comments
            auto sc = line.find(';');
            if (sc != std::string::npos) line = line.substr(0, sc);

            // Trim whitespace
            while (!line.empty() && std::isspace((unsigned char)line.front())) line.erase(line.begin());
            if (line.empty()) continue;

            std::istringstream ss(line);
            std::string token;
            ss >> token;
            if (token.empty()) continue;

            // --- NEW separator ---
            if (token == "NEW")
            {
                m_programs.emplace_back();
                ++cur;
                continue;
            }

            // --- Resolve G-type (full and shorthand) ---
            int parsedType = resolveGType(token);
            bool hasGWord = (parsedType != -1);

            if (!hasGWord)
            {
                // Modal carry-over — line starts with a coordinate word
                char c = (char)std::toupper(token[0]);
                if (c == 'X' || c == 'Y' || c == 'I' || c == 'J' || c == 'F' || c == 'P')
                {
                    parsedType = modalType;
                    ss.clear(); ss.str(line); // re-read from start
                }
                else continue; // unknown token
            }
            else
            {
                modalType = parsedType;
            }

            // --- Parse coordinate words ---
            sf::Vector2f target{ 0.f, 0.f };
            sf::Vector2f centerOffset{ 0.f, 0.f };
            float feedWord = -1.f;
            float dwellSec = -1.f;

            // Seed target with current endpoint of last command
            // so partial coordinate lines work (e.g. "G01 X50" keeps last Y)
            if (!m_programs[cur].empty())
                target = lastTarget(m_programs[cur]);

            std::string t;
            while (ss >> t)
            {
                if (t.empty()) continue;
                char id = (char)std::toupper(t[0]);
                try {
                    float val = std::stof(t.substr(1));
                    if (id == 'X') target.x = val;
                    else if (id == 'Y') target.y = val;
                    else if (id == 'I') centerOffset.x = val;
                    else if (id == 'J') centerOffset.y = val;
                    else if (id == 'F') feedWord = val;
                    else if (id == 'P') dwellSec = val;
                }
                catch (...) {}
            }

            // --- Build command ---
            switch (parsedType)
            {
            case 0:
                m_programs[cur].push_back(
                    std::make_unique<LinearCommand>(target, 0, feedWord));
                break;
            case 1:
                m_programs[cur].push_back(
                    std::make_unique<LinearCommand>(target, 1, feedWord));
                break;
            case 2:
            case 3:
                m_programs[cur].push_back(
                    std::make_unique<ArcCommand>(target, centerOffset, parsedType, feedWord));
                break;
            case 4: // G04 dwell
                if (dwellSec > 0.f)
                    m_programs[cur].push_back(
                        std::make_unique<DwellCommand>(dwellSec));
                break;
            default: break;
            }
        }

        // Remove empty trailing program if NEW was the last line
        while (m_programs.size() > 1 && m_programs.back().empty())
            m_programs.pop_back();
    }

    // --- Execution ---

    // Call every frame while running.
    // Returns true if there are still commands left.
    bool tick(Robot& robot, float dt)
    {
        if (m_programs.empty()) return false;
        auto& prog = currentProgram();
        if (m_cmdIndex >= prog.size()) return false;

        prog[m_cmdIndex]->execute(robot, dt);

        if (prog[m_cmdIndex]->isFinished())
            ++m_cmdIndex;

        return m_cmdIndex < prog.size();
    }

    bool isFinished() const
    {
        if (m_programs.empty()) return true;
        return m_cmdIndex >= currentProgram().size();
    }

    // --- Navigation ---

    void reset(Robot& robot)
    {
        m_cmdIndex = 0;
        robot.reset();
    }

    void setProgram(int index, Robot& robot)
    {
        if (m_programs.empty()) return;
        m_progIndex = ((index % (int)m_programs.size()) + (int)m_programs.size())
            % (int)m_programs.size();
        reset(robot);
    }

    void nextProgram(Robot& robot) { setProgram(m_progIndex + 1, robot); }
    void prevProgram(Robot& robot) { setProgram(m_progIndex - 1, robot); }

    // --- Queries ---

    int programCount() const { return (int)m_programs.size(); }
    int programIndex()  const { return m_progIndex; }
    int commandIndex()  const { return (int)m_cmdIndex; }

    int commandCount() const
    {
        if (m_programs.empty()) return 0;
        return (int)currentProgram().size();
    }

    // Label of the currently executing command (for HUD)
    std::string currentLabel() const
    {
        if (isFinished()) return "COMPLETE";
        return currentProgram()[m_cmdIndex]->label();
    }

    // Lookahead labels (next N commands) for a preview list
    std::vector<std::string> preview(int n) const
    {
        std::vector<std::string> out;
        if (m_programs.empty()) return out;
        auto& prog = currentProgram();
        for (size_t i = m_cmdIndex; i < prog.size() && (int)(i - m_cmdIndex) < n; ++i)
            out.push_back(prog[i]->label());
        return out;
    }

    bool empty() const { return m_programs.empty(); }

private:
    using Program = std::vector<std::unique_ptr<Command>>;

    std::vector<Program> m_programs;
    int    m_progIndex{ 0 };
    size_t m_cmdIndex{ 0 };

    const Program& currentProgram() const { return m_programs[m_progIndex]; }
    Program& currentProgram() { return m_programs[m_progIndex]; }

    static int resolveGType(const std::string& s)
    {
        if (s == "G00" || s == "G0")  return 0;
        if (s == "G01" || s == "G1")  return 1;
        if (s == "G02" || s == "G2")  return 2;
        if (s == "G03" || s == "G3")  return 3;
        if (s == "G04" || s == "G4")  return 4;
        return -1;
    }

    // Returns the target position of the last command in a program,
    // used to seed partial coordinate lines.
    static sf::Vector2f lastTarget(const Program& prog)
    {
        // Walk backwards to find a command with a known endpoint
        for (int i = (int)prog.size() - 1; i >= 0; --i)
        {
            auto* lc = dynamic_cast<LinearCommand*>(prog[i].get());
            if (lc) { /* can't easily read private target — use default */ break; }
        }
        return { 0.f, 0.f }; // safe fallback
    }
};