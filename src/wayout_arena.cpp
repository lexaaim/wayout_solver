#include "wayout_arena.h"
#include <algorithm>

using namespace std;
using namespace WayOut;

// two methods, that recursively populate the set of index tile's neighbors
void Arena::insert_neighbor(set<uint> & nbs, const uint index) const {
    if (m_tiles[index] == Tile::Plus) {
        collect_neighbors(nbs, index);
    } else if (m_tiles[index] != Tile::None
            && m_tiles[index] != Tile::OneDot) {
        nbs.insert(index);
    }
}

void Arena::collect_neighbors(set<uint> & nbs, const uint index) const {
    if (nbs.count(index) != 0) { return; }
    if (m_tiles[index] == Tile::None) { return; }
    nbs.insert(index);

    const bool is_leftmost   = index % m_width == 0;
    const bool is_rightmost  = index % m_width + 1 == m_width;
    const bool is_topmost    = index / m_width == 0;
    const bool is_bottommost = index / m_width + 1 == m_height;

    if (m_tiles[index] != Tile::VArrow && m_tiles[index] != Tile::DArrow) {
        if (!is_leftmost)  { insert_neighbor(nbs, index - 1); }
        if (!is_rightmost) { insert_neighbor(nbs, index + 1); }
    }

    if (m_tiles[index] != Tile::HArrow && m_tiles[index] != Tile::DArrow) {
        if (!is_topmost)    { insert_neighbor(nbs, index - m_width); }
        if (!is_bottommost) { insert_neighbor(nbs, index + m_width); }
    }

#ifdef WAYOUT2
    if (m_tiles[index] != Tile::VArrow && m_tiles[index] != Tile::HArrow) {
        if (!is_leftmost && !is_topmost)     { insert_neighbor(nbs, index - m_width - 1); }
        if (!is_rightmost && !is_bottommost) { insert_neighbor(nbs, index + m_width + 1); }
    }
#endif
}

bool Arena::initialize(const vector<pair<Tile, State>> & level,
        uint width, uint height)
{
    auto cnt = width * height;
    if (cnt == 0 || cnt > MAX_COUNT || level.size () != cnt) {
        return false;
    }

    m_width = width;
    m_height = height;
    uint index = 0;
    for (const auto & [site, state]: level) {
        m_tiles.push_back(site);
        m_states[index++] = state == State::Up;
    }
    m_init_states = m_states;
    m_init_tiles = m_tiles;

    // for each tile get all tiles, that it triggers
    for (uint i = 0; i < level.size(); i++) {
        if (m_tiles[i] == Tile::DblDot) {
            m_dbldot_states.first  |= !m_states[i];
            m_dbldot_states.second |=  m_states[i];
            m_dbldots.push_back(i);
        }

        set<uint> nbs;
        collect_neighbors(nbs, i);
        nbs.erase(i);
        vector<uint> tmp{ nbs.begin(), nbs.end() };
        m_neighbors.push_back(move(tmp));
    }

    preliminary_calculations();
    return true;
}

void Arena::preliminary_calculations() {
    // mark each tile, that triggers some double dot tiles
    for (uint i = 0; i < count(); i++) {
        bool isdd = false;
        for (uint j: m_neighbors[i]) {
            if (m_tiles[j] == Tile::DblDot) { isdd = true; break; }
        }
        if (m_tiles[i] == Tile::DblDot) { isdd = true; }
        if (isdd) m_has_dbldots_neighbors[i] = true;
    }

    // for each tile get all the tiles, that trigger it
    for(uint i = 0; i < count(); i++) {
        m_rev_neighbors.push_back({});
        if (m_tiles[i] == Tile::Yellow) {
            for(uint j = 0; j < count(); j++) {
                if (std::count(m_neighbors[j].begin(), m_neighbors[j].end(), i) > 0) {
                    m_rev_neighbors.back().push_back(j);
                }
            }
        }
    }
}

void Arena::restore_initial_states() {
    m_tiles  = m_init_tiles;
    m_states = m_init_states;
}

void Arena::flip(const uint index) {
    if (m_tiles[index] == Tile::DblDot) {
        flip_dbldots(index);
    } else {
        m_states.flip(index);
    }
}

void Arena::flip_neighbor(const uint index) {
    if (m_tiles[index] == Tile::Yellow) {
        m_tiles[index] = Tile::Normal;
    } else if (m_tiles[index] == Tile::DblDot) {
        flip_dbldots(index);
    } else {
        m_states.flip(index);
    }
}

void Arena::flip_dbldots(const uint index) {
    if (m_dbldots_were_flipped) { return; }

    const bool new_state = !m_states[index];
    for(const auto di: m_dbldots) {
        m_states.set(di, new_state);
    }
    m_dbldots_were_flipped = true;
}

void Arena::toggle(const uint index) {
    m_dbldots_were_flipped = false;

    flip(index);
    for(const auto & ni: m_neighbors[index]) {
        flip_neighbor(ni);
    }
}
