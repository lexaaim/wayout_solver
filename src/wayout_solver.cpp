#include "wayout_solver.h"
#include "wayout_formatter.h"
#include "wayout_arena.h"
#include "wayout_solving_algorithm.h"

#include <istream>
#include <ostream>
#include <algorithm>

using namespace std;
using namespace WayOut;

Solver::Solver() : m_arena{ make_unique<Arena>() }, m_result{} {
}

Solver::~Solver() = default;

pair<bool, uint> Solver::solve() {
    {
        SolvingAlgorithm salgorithm;

        m_arena = salgorithm.solve(move(m_arena));
        m_result = salgorithm.result();
    }

    return make_pair(!m_result.empty(), m_result.size());
}

bool Solver::read_level_data(std::istream & stream, const char iformat[]) {
    string line;
    uint height = 0, width = 0;
    vector<pair<Tile, State>> level;

    while (stream >> line) {
        if (line.empty()) { continue; }
        if (width == 0) { width = static_cast<uint>(line.length()); }
        if (width != line.length()) { return false; }

        for(const auto ch: line) {
            auto state = Formatter::encode_state(iformat, ch);
            auto tile  = Formatter::encode_tail(iformat, ch);
            if (!tile.has_value()) { return false; }
            level.push_back(make_pair(*tile, state));
        }
        height++;
    }
    if (level.empty()) { return false; }

    return m_arena->initialize(level, width, height);
}

void Solver::print_solution(ostream & stream, const char oformat[4], bool print_numbers) const {
    vector<uint> repeated_toggles;
    uint order = 1;
    auto fn_format_result_string = [&](const vector<uint> & toggles) {
        string sstage;
        sstage.reserve(m_arena->count());

        // draw the level basics
        auto fn_filler = [&oformat](Tile s) {
            return oformat[(s == Tile::None ? Formatter::O_NONE : Formatter::O_REST)];
        };
        transform(begin(m_arena->tiles()), end(m_arena->tiles()),
                  back_inserter(sstage), fn_filler);

        // mark the toggles
        for (const auto ind: toggles) {
            if (sstage[ind] != oformat[Formatter::O_REST]) {
                repeated_toggles.push_back(ind);
                continue;
            }

            sstage[ind] = (print_numbers ? Formatter::to_char(order++)
                                         : oformat[Formatter::O_TOGG]);
        }

        return move(sstage);
    };

    string sstage1{fn_format_result_string(m_result)};
    string sstage2{fn_format_result_string(repeated_toggles)};

    const uint & width = m_arena->width();
    for(uint i = 0; i < m_arena->height(); i++) {
        stream << sstage1.substr(width * i, width);
        if (!repeated_toggles.empty()) {
            stream << " | " << sstage2.substr(width * i, width);
        }
        stream << '\n';
    }
}

