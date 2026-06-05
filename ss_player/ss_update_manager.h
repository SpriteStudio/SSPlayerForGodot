#pragma once

#include "ss_player_node_2d.h"
#include <vector>
#include <mutex>
#include <shared_mutex>

class SsUpdateManager {
private:
    std::vector<SpriteStudioPlayer2D*> _players;
    std::shared_mutex _mutex;

    SsUpdateManager() {}

public:
    static SsUpdateManager& get();

    void register_player(SpriteStudioPlayer2D* player);
    void unregister_player(SpriteStudioPlayer2D* player);
    bool is_player_registered(uint64_t instance_id);

    void update_all(float delta_seconds, bool physics);
};
