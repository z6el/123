#pragma once

#include <src/include.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

struct ActorData {
    uintptr_t       address;
    engine::vec3_t  position;
    engine::vec3_t  head_position;
    std::string     name;
    std::string     prefab_name;
    unsigned long   team_id;
    int             flags;
    int             lifestate;
    float           health;
    float           max_health;
    float           distance;
    bool            is_sleeping;
    bool            is_wounded;
    bool            is_npc;
    bool            is_player;
};

namespace esp {
    inline bool  enabled         = true;
    inline bool  draw_boxes       = true;
    inline bool  draw_box_fill    = false;
    inline bool  draw_corner_box  = false;
    inline bool  draw_names       = true;
    inline bool  draw_distance    = false;
    inline bool  draw_snaplines   = false;
    inline bool  draw_health_bar  = false;
    inline bool  show_sleepers   = false;
    inline bool  show_wounded    = true;
    inline bool  show_npcs       = true;
    inline float max_distance    = 150.0f;

    // Crosshair
    inline bool  crosshair_enabled = false;
    inline float crosshair_size = 1.0f;
    inline bool  crosshair_spin = false;
    inline float crosshair_spin_speed = 1.0f;
    inline float crosshair_normal_size = 10.0f;
    inline float crosshair_normal_gap = 5.0f;
    inline float crosshair_normal_gapTween = 0.0f;
    inline float crosshair_normal_gapSpeed = 1.0f;
    inline float crosshair_normal_thickness = 1.0f;
    inline int   crosshair_normal_styleIdx = 0; // 0 = Static, 1 = Pulse
    inline float crosshair_normal_baseSpeed = 100.0f;
    inline float crosshair_normal_fadeDuration = 1.0f;
    // Crosshair color (RGBA 0-1)
    inline float crosshair_color[4] = { 1.f, 1.f, 1.f, 1.f };

    // Colors (RGBA 0-1) - per-entity-type for box/name
    inline float col_enemy[4]     = { 1.f, 1.f, 1.f, 1.f };
    inline float col_team[4]      = { 0.39f, 1.f, 0.59f, 1.f };
    inline float col_sleeper[4]   = { 0.55f, 0.55f, 0.55f, 0.78f };
    inline float col_wounded[4]   = { 1.f, 0.65f, 0.f, 1.f };
    inline float col_npc[4]       = { 0.78f, 0.55f, 1.f, 1.f };
    inline float col_box_fill[4]  = { 1.f, 1.f, 1.f, 0.2f };
    inline float col_name[4]      = { 1.f, 1.f, 1.f, 1.f };
    inline float col_distance[4]  = { 0.78f, 0.78f, 0.78f, 1.f };
    inline float col_snapline[4]  = { 1.f, 1.f, 1.f, 0.24f };


    inline int   update_interval_ms = 1;
    inline int   player_count    = 0;

    void update();
    void render();
    void start_update_thread();
    void stop_update_thread();
}

extern std::shared_ptr<std::vector<ActorData>> g_actors;
extern std::mutex               g_mutex;
extern engine::matrix_4x4_t     g_view_matrix;
extern engine::vec3_t           g_camera_pos;
extern unsigned long            g_local_team;
extern uint64_t                 g_local_player;
extern uint64_t                 g_cam_buffer;

bool w2s(const engine::vec3_t& world, const engine::matrix_4x4_t& vm, float sw, float sh, engine::vec2_t& out);

