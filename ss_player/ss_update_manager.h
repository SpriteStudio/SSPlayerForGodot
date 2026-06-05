#pragma once

#include "ss_player_node_2d.h"
#include "ss_player_node_2d.h"
#include <vector>

class SsUpdateManager {
private:
    std::vector<SpriteStudioPlayer2D*> _players;

    SsUpdateManager() {}

public:
    static SsUpdateManager& get() {
        static SsUpdateManager instance;
        return instance;
    }

    void register_player(SpriteStudioPlayer2D* player) {
        _players.push_back(player);
    }

    void unregister_player(SpriteStudioPlayer2D* player) {
        auto it = std::find(_players.begin(), _players.end(), player);
        if (it != _players.end()) {
            _players.erase(it);
        }
    }

    void update_all(float delta_seconds, bool physics);
};
