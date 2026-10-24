#include "esp.h"

static std::thread       g_update_thread;
static std::atomic<bool> g_update_running{ false };

void esp::start_update_thread() {
    if (g_update_running.exchange(true)) return;
    g_update_thread = std::thread([]() {
        while (g_update_running) {
            esp::update();
            Sleep(1);
        }
    });
}

void esp::stop_update_thread() {
    g_update_running = false;
    if (g_update_thread.joinable())
        g_update_thread.join();
}

static const char* g_npc_classes[] = {
    "NPCPlayer", "ScientistNPC", "HTNPlayer", "BasePlayer",
    "NPCMurderer", "HumanNPC", "GingerbreadNPC"
};

static ImU32 col_from_float(const float* c) {
    return IM_COL32((int)(c[0]*255), (int)(c[1]*255), (int)(c[2]*255), (int)(c[3]*255));
}

std::shared_ptr<std::vector<ActorData>> g_actors = std::make_shared<std::vector<ActorData>>();
std::mutex              g_mutex;
engine::matrix_4x4_t    g_view_matrix = {};
engine::vec3_t          g_camera_pos = {};
unsigned long           g_local_team = 0;
uint64_t                g_local_player = 0;
uint64_t                g_cam_buffer = 0;

static engine::matrix_4x4_t g_view_matrix_cached = {};
static std::mutex           g_vm_mutex;
static std::vector<ActorData> g_back_buf;
static LARGE_INTEGER        g_perf_freq = {};
static LARGE_INTEGER        g_last_update = {};
static DWORD                g_last_cam_check = 0;
static bool                 g_cam_resolved = false;

static double perf_elapsed_ms() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - g_last_update.QuadPart) * 1000.0 / (double)g_perf_freq.QuadPart;
}

bool w2s(const engine::vec3_t& world, const engine::matrix_4x4_t& vm, float sw, float sh, engine::vec2_t& out)
{
    float w = vm._14 * world.x + vm._24 * world.y + vm._34 * world.z + vm._44;
    if (w < 0.001f) return false;
    float invW = 1.f / w;
    float sx = vm._11 * world.x + vm._21 * world.y + vm._31 * world.z + vm._41;
    float sy = vm._12 * world.x + vm._22 * world.y + vm._32 * world.z + vm._42;
    out.x = (sw * 0.5f) + (sw * 0.5f) * sx * invW;
    out.y = (sh * 0.5f) - (sh * 0.5f) * sy * invW;
    return (out.x >= -50.f && out.x <= sw + 50.f && out.y >= -50.f && out.y <= sh + 50.f);
}

static bool is_npc_class(const std::string& name)
{
    for (auto& n : g_npc_classes)
        if (name == n) return true;
    return false;
}

static uint64_t resolve_camera_buffer()
{
    uint64_t cam_type = m.read<uint64_t>(globals::game_assembly + offsets::base_camera);
    if (!cam_type) return 0;
    uint64_t cam_sf = m.read<uint64_t>(cam_type + offsets::base_camera_class::static_fields);
    if (!cam_sf) return 0;
    uint64_t cam_wrapper = m.read<uint64_t>(cam_sf + offsets::base_camera_class::wrapper_class);
    if (!cam_wrapper) return 0;
    return m.read<uint64_t>(cam_wrapper + offsets::base_camera_class::parent_static_fields);
}

void esp::update()
{
    if (g_perf_freq.QuadPart == 0)
        QueryPerformanceFrequency(&g_perf_freq);

    if (esp::update_interval_ms > 0 && perf_elapsed_ms() < (double)esp::update_interval_ms)
        return;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    g_last_update = now;

    DWORD tick = GetTickCount();
    if (!g_cam_resolved || (tick - g_last_cam_check > 500))
    {
        g_last_cam_check = tick;
        uint64_t test = resolve_camera_buffer();
        if (test)
        {
            g_cam_buffer = test;
            g_cam_resolved = true;
        }
    }

    if (!g_cam_buffer) return;

    engine::matrix_4x4_t vm = m.read<engine::matrix_4x4_t>(g_cam_buffer + offsets::base_camera_class::matrix);
    g_camera_pos = m.read<engine::vec3_t>(g_cam_buffer + offsets::base_camera_class::position);

    BaseNetworkable* net = BaseNetworkable::get_base_networkable();
    if (!net) return;

    auto list = net->get_object_list();
    if (!list.array || list.count == 0 || list.count > 50000) return;

    g_back_buf.clear();
    g_back_buf.reserve(384);

    for (uint64_t i = 0; i < list.count; ++i)
    {
        uintptr_t ent = m.read<uintptr_t>(list.array + 0x20 + i * 8);
        if (!ent || ent < 0x10000000) continue;

        BaseEntity* base = (BaseEntity*)ent;
        std::string cls = base->get_class_name();
        if (cls.empty()) continue;

        bool npc = is_npc_class(cls);
        bool player = (cls == "BasePlayer");
        if (!player && !npc) continue;

        BasePlayer* bp = (BasePlayer*)ent;
        auto ls = bp->lifestate();
        if (ls != BaseCombatEntity::Alive) continue;

        PlayerModel* mdl = bp->playerModel();
        if (!mdl) continue;

        engine::vec3_t pos = mdl->position();
        if (pos.empty()) continue;

        int flags = (int)bp->playerFlags();
        bool sleeping = (flags & (int)BasePlayer::PlayerFlags::Sleeping) != 0;
        bool wounded = (flags & (int)BasePlayer::PlayerFlags::Wounded) != 0;
        bool connected = (flags & (int)BasePlayer::PlayerFlags::Connected) != 0;

        if (!connected && !sleeping) continue;

        ActorData a;
        a.address = ent;
        a.position = pos;
        a.head_position = engine::vec3_t(pos.x, pos.y + 1.6f, pos.z);
        a.name = bp->_displayName();
        a.team_id = bp->currentTeam();
        a.flags = flags;
        a.lifestate = 0;
        a.health = bp->_health();
        a.max_health = bp->_maxHealth();
        a.is_sleeping = sleeping;
        a.is_wounded = wounded;
        a.is_npc = npc;
        a.is_player = player;
        a.distance = pos.Distance(g_camera_pos);

        g_back_buf.push_back(std::move(a));
        if (g_back_buf.size() >= 384) break;
    }

    float best = FLT_MAX;
    unsigned long closest_team = 0;
    uintptr_t closest_ent = 0;

    for (const auto& a : g_back_buf)
    {
        if (a.is_npc || a.is_sleeping) continue;
        if (a.distance < best)
        {
            best = a.distance;
            closest_team = a.team_id;
            closest_ent = a.address;
        }
    }

    if (best < 5.f)
    {
        g_local_team = closest_team;
        g_local_player = closest_ent;
    }

    {
        std::lock_guard<std::mutex> lk(g_vm_mutex);
        g_view_matrix_cached = vm;
    }

    {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_actors = std::make_shared<std::vector<ActorData>>(std::move(g_back_buf));
        g_back_buf.reserve(384);
        g_view_matrix = vm;
    }
}

void esp::render()
{
    if (!enabled) return;

    engine::matrix_4x4_t vm;
    if (g_cam_buffer) {
        vm = m.read<engine::matrix_4x4_t>(g_cam_buffer + offsets::base_camera_class::matrix);
    } else {
        std::lock_guard<std::mutex> lk(g_vm_mutex);
        vm = g_view_matrix_cached;
    }

    float sw = (float)overlay::width;
    float sh = (float)overlay::height;
    if (sw <= 0.f || sh <= 0.f) return;

    ImDrawList* draw = ImGui::GetForegroundDrawList();

    std::shared_ptr<std::vector<ActorData>> snapshot;
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        snapshot = g_actors;
    }
    if (!snapshot) return;

    int count = 0;
    for (const auto& a : *snapshot)
    {
        if (!a.is_player && !a.is_npc) continue;
        if (a.is_sleeping && !show_sleepers) continue;
        if (a.is_wounded && !show_wounded)  continue;
        if (a.is_npc && !show_npcs)     continue;
        if (max_distance > 0.f && a.distance > max_distance) continue;

        engine::vec2_t feet, head;
        if (!w2s(a.position, vm, sw, sh, feet)) continue;
        if (!w2s(a.head_position, vm, sw, sh, head)) continue;

        float h_diff = fabsf(feet.y - head.y);
        if (h_diff < 10.f) h_diff = 10.f;
        float w_diff = h_diff * 0.5f;

        float bx = head.x - (w_diff * 0.5f);
        float by = head.y;

        ImU32 col = col_from_float(esp::col_enemy);

        if (draw_names && !a.name.empty())
        {
            if (a.is_npc) col = col_from_float(esp::col_npc);
            else if (a.is_wounded) col = col_from_float(esp::col_wounded);
            else if (a.is_sleeping) col = col_from_float(esp::col_sleeper);
            else if (a.team_id == g_local_team && a.team_id != 0) col = col_from_float(esp::col_team);
        }

        ImVec2 tl(bx, by), br(bx + w_diff, by + h_diff);
        float corner_sz = fminf(w_diff, h_diff) * 0.25f;

        if (draw_box_fill)
            draw->AddRectFilled(tl, br, col_from_float(esp::col_box_fill));


        if (draw_corner_box) {
            draw->AddLine(ImVec2(tl.x, tl.y), ImVec2(tl.x + corner_sz, tl.y), col, 1.f);
            draw->AddLine(ImVec2(tl.x, tl.y), ImVec2(tl.x, tl.y + corner_sz), col, 1.f);
            draw->AddLine(ImVec2(br.x - corner_sz, br.y), ImVec2(br.x, br.y), col, 1.f);
            draw->AddLine(ImVec2(br.x, br.y - corner_sz), ImVec2(br.x, br.y), col, 1.f);
            draw->AddLine(ImVec2(tl.x, br.y - corner_sz), ImVec2(tl.x, br.y), col, 1.f);
            draw->AddLine(ImVec2(tl.x, br.y), ImVec2(tl.x + corner_sz, br.y), col, 1.f);
            draw->AddLine(ImVec2(br.x - corner_sz, tl.y), ImVec2(br.x, tl.y), col, 1.f);
            draw->AddLine(ImVec2(br.x, tl.y), ImVec2(br.x, tl.y + corner_sz), col, 1.f);
        }
        else if (draw_boxes)
            draw->AddRect(tl, br, col, 0.f, 0, 1.f);

        if (draw_names && !a.name.empty())
        {
            ImVec2 txt_sz = ImGui::CalcTextSize(a.name.c_str());
            draw->AddText(ImVec2(head.x - txt_sz.x * 0.5f, head.y - txt_sz.y - 4.f), col_from_float(esp::col_name), a.name.c_str());
        }

        if (draw_distance)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.0fm", a.distance);
            ImVec2 txt_sz = ImGui::CalcTextSize(buf);
            draw->AddText(ImVec2(feet.x - txt_sz.x * 0.5f, feet.y + 4.f), col_from_float(esp::col_distance), buf);
        }

        if (draw_snaplines)
            draw->AddLine(ImVec2(sw * 0.5f, sh), ImVec2(feet.x, feet.y), col_from_float(esp::col_snapline), 1.f);

        ++count;
    }

    player_count = count;
}