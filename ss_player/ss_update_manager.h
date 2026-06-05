#pragma once

#include "ss_player_node_2d.h"
#include <vector>
#include <mutex>

class SsUpdateManager {
private:
    std::vector<SpriteStudioPlayer2D*> _players;
    std::mutex _mutex;

    uint64_t _last_process_frame = 0;
    uint64_t _last_physics_frame = 0;

    SsUpdateManager() {}

public:
    static SsUpdateManager& get() {
        static SsUpdateManager instance;
        return instance;
    }

    void register_player(SpriteStudioPlayer2D* player) {
        std::lock_guard<std::mutex> lock(_mutex);
        _players.push_back(player);
    }

    void unregister_player(SpriteStudioPlayer2D* player) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = std::find(_players.begin(), _players.end(), player);
        if (it != _players.end()) {
            _players.erase(it);
        }
    }

    void update_all(float delta_seconds, bool physics);
};
