#include "ss_update_manager.h"
#include "ss_internal_player.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#else
#include "core/config/engine.h"
#include "core/object/worker_thread_pool.h"
#endif

using namespace godot;

static void _get_frame_data_task(void* p_userdata, uint32_t p_index) {
    auto** players = static_cast<SpriteStudioPlayer2D**>(p_userdata);
    SpriteStudioPlayer2D* player = players[p_index];
    if (player && player->_get_internal_player()) {
        player->_get_internal_player()->get_frame_data_sync();
    }
}

void SsUpdateManager::update_all(float delta_seconds, bool physics) {
    Engine* engine = Engine::get_singleton();
    uint64_t current_frame = physics ? engine->get_physics_frames() : engine->get_process_frames();
    
    if (physics) {
        if (_last_physics_frame == current_frame) return;
        _last_physics_frame = current_frame;
    } else {
        if (_last_process_frame == current_frame) return;
        _last_process_frame = current_frame;
    }
    
    std::vector<SpriteStudioPlayer2D*> active_players;
    
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto player : _players) {
            if (player->is_inside_tree() && 
                player->get_animation_process_mode() == (physics ? SpriteStudioPlayer2D::ANIMATION_PROCESS_PHYSICS : SpriteStudioPlayer2D::ANIMATION_PROCESS_IDLE)) {
                active_players.push_back(player);
            }
        }
    }
    
    if (active_players.empty()) return;
    
    std::vector<SpriteStudioPlayer2D*> pending_players;
    pending_players.reserve(active_players.size());
    
    // Pass A: Prepare
    for (auto player : active_players) {
        player->_push_coverage_screen_scale();
        if (player->_get_internal_player()->prepare_frame(delta_seconds)) {
            pending_players.push_back(player);
        }
    }
    
    int n = pending_players.size();
    if (n == 0) return;
    
    // Pass B: Parallel get_frame_data
    if (n >= 2) {
        WorkerThreadPool* pool = WorkerThreadPool::get_singleton();
        int64_t group_id = pool->add_native_group_task(&_get_frame_data_task, pending_players.data(), n, -1, false, "SsGetFrameData");
        pool->wait_for_group_task_completion(group_id);
    } else {
        // Sequential fallback
        pending_players[0]->_get_internal_player()->get_frame_data_sync();
    }
    
    // Pass C: Consume
    for (auto player : pending_players) {
        player->_get_internal_player()->consume_frame(delta_seconds);
    }
}
