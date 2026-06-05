#include "ss_update_manager.h"
#include "ss_internal_player.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#else
#include "core/config/engine.h"
#include "core/object/worker_thread_pool.h"
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
using namespace godot;
#endif

struct PendingPlayer {
    SpriteStudioPlayer2D* ptr;
    uint64_t id;
};

static void _get_frame_data_task(void* p_userdata, uint32_t p_index) {
    auto* pending = static_cast<PendingPlayer*>(p_userdata);
    PendingPlayer& player = pending[p_index];
    if (SsUpdateManager::get().is_player_registered(player.id) && player.ptr && player.ptr->_get_internal_player()) {
        player.ptr->_get_internal_player()->get_frame_data_sync();
    }
}

void SsUpdateManager::register_player(SpriteStudioPlayer2D* player) {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    if (std::find(_players.begin(), _players.end(), player) == _players.end()) {
        _players.push_back(player);
    }
}

void SsUpdateManager::unregister_player(SpriteStudioPlayer2D* player) {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    auto it = std::find(_players.begin(), _players.end(), player);
    if (it != _players.end()) {
        _players.erase(it);
    }
}

bool SsUpdateManager::is_player_registered(uint64_t instance_id) {
    std::shared_lock<std::shared_mutex> lock(_mutex);
    for (auto p : _players) {
        if (static_cast<uint64_t>(p->get_instance_id()) == instance_id) {
            return true;
        }
    }
    return false;
}

void SsUpdateManager::update_all(float delta_seconds, bool physics) {
    Engine* engine = Engine::get_singleton();
    uint64_t current_frame = physics ? engine->get_physics_frames() : engine->get_process_frames();
    
    std::vector<SpriteStudioPlayer2D*> active_players;
    
    {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        for (auto player : _players) {
            if (!player->can_process()) continue;
            if (player->get_animation_process_mode() != (physics ? SpriteStudioPlayer2D::ANIMATION_PROCESS_PHYSICS : SpriteStudioPlayer2D::ANIMATION_PROCESS_IDLE)) continue;
            
            uint64_t& last_frame = physics ? player->_last_updated_physics_frame : player->_last_updated_process_frame;
            if (last_frame == current_frame) continue;
            
            last_frame = current_frame;
            active_players.push_back(player);
        }
    }
    
    if (active_players.empty()) return;
    
    std::vector<PendingPlayer> pending_players;
    pending_players.reserve(active_players.size());
    
    // Pass A: Prepare
    for (auto player : active_players) {
        player->_push_coverage_screen_scale();
        if (player->_get_internal_player()->prepare_frame(delta_seconds)) {
            pending_players.push_back({ player, static_cast<uint64_t>(player->get_instance_id()) });
        }
    }
    
    int n = pending_players.size();
    if (n == 0) return;
    
    // Pass B: Parallel get_frame_data
    if (n >= 8) {
        WorkerThreadPool* pool = WorkerThreadPool::get_singleton();
        int64_t group_id = pool->add_native_group_task(&_get_frame_data_task, pending_players.data(), n, -1, false, "SsGetFrameData");
        pool->wait_for_group_task_completion(group_id);
    } else {
        // Sequential fallback
        for (auto& pending : pending_players) {
            if (is_player_registered(pending.id)) {
                pending.ptr->_get_internal_player()->get_frame_data_sync();
            }
        }
    }
    
    // Pass C: Consume
    for (auto& pending : pending_players) {
        if (is_player_registered(pending.id)) {
            pending.ptr->_get_internal_player()->consume_frame(delta_seconds);
        }
    }
}
