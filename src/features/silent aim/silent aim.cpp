#include "silent aim.h"
#include <src/features/esp/esp.h>

static engine::vec3_t calc_angle(const engine::vec3_t& local, const engine::vec3_t& target)
{
    engine::vec3_t dir = target - local;
    float distance = sqrtf(dir.x * dir.x + dir.z * dir.z);
    float pitch = -RAD2DEG(atan2(dir.y, distance));
    float yaw = RAD2DEG(atan2(dir.x, dir.z));
    return engine::vec3_t(pitch, yaw, 0.0f);
}

void silent_aim::render()
{
    if (!silent_aim::draw_fov || !silent_aim::enabled) return;

    float sw = (float)overlay::width;
    float sh = (float)overlay::height;
    if (sw <= 0.f || sh <= 0.f) return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    draw->AddCircle(ImVec2(sw * 0.5f, sh * 0.5f), silent_aim::fov,
        IM_COL32(255, 255, 255, 120), 64, 1.0f);
}

void silent_aim::update()
{
    if (!silent_aim::enabled) return;
    if (!g_local_player) return;

    BasePlayer* local = (BasePlayer*)g_local_player;
    PlayerEyes* eyes = local->eyesValue();
    if (!eyes) return;

    float sw = (float)overlay::width;
    float sh = (float)overlay::height;

    float          best_fov = silent_aim::fov;
    engine::vec3_t best_target_pos;
    bool           found_target = false;

    std::shared_ptr<std::vector<ActorData>> snapshot;
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        snapshot = g_actors;
    }
    if (!snapshot) return;

    for (const auto& a : *snapshot) {
        if (a.address == g_local_player) continue;
        if (a.is_sleeping && !silent_aim::target_sleepers) continue;
        if (a.is_npc && !silent_aim::target_npcs)     continue;
        if (a.team_id == g_local_team && a.team_id != 0)   continue;

        engine::vec2_t screen_pos;
        if (!w2s(a.head_position, g_view_matrix, sw, sh, screen_pos)) continue;

        float dx = screen_pos.x - (sw * 0.5f);
        float dy = screen_pos.y - (sh * 0.5f);
        float fov = sqrtf(dx * dx + dy * dy);

        if (fov < best_fov) {
            best_fov = fov;
            best_target_pos = a.head_position;
            found_target = true;
        }
    }

    if (found_target && silent_aim::silentaim) {
        engine::vec3_t angle = calc_angle(g_camera_pos, best_target_pos);
        angle.clamp();
        eyes->SetBodyRotation(angle.Quaternion());
    }
}