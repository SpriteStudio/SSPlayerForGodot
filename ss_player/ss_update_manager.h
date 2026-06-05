#pragma once

#include "ss_player_node_2d.h"
#include <vector>
#include <mutex>

class SsUpdateManager {
private:
    std::vector<SpriteStudioPlayer2D*> _players;
    std::mutex _mutex;

    SsUpdateManager() {}

public:
    static SsUpdateManager& get() {
        static SsUpdateManager instance;
        return instance;
    }

    void register_player(SpriteStudioPlayer2D* player);
    void unregister_player(SpriteStudioPlayer2D* player);
    bool is_player_registered(SpriteStudioPlayer2D* player);

    void update_all(float delta_seconds, bool physics);
};
