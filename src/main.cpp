#include <src/include.h>
#include "features/esp/esp.h"
#include "features/silent aim/silent aim.h"
#include "features/misc/misc.h"
#include "overlay/overlay.h"
#include "main.h"
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

namespace {
    int startup_failure(const char* message) {
        fprintf(stderr, "Startup failed: %s\n", message);
        fflush(stderr);

        DWORD console_mode = 0;
        if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &console_mode)) {
            fprintf(stderr, "Press Enter to close...\n");
            fflush(stderr);
            getchar();
        }
        return EXIT_FAILURE;
    }
}

int main() {
    printf("Starting Rust External...\n");

    if (!m.running(L"RustClient.exe")) {
        printf("Launching Rust through Steam...\n");
        fflush(stdout);

        SHELLEXECUTEINFOW launch = {};
        launch.cbSize = sizeof(launch);
        launch.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
        launch.lpVerb = L"open";
        launch.lpFile = L"steam://rungameid/252490";
        launch.nShow = SW_SHOWNORMAL;
        if (!ShellExecuteExW(&launch)) {
            const DWORD error = GetLastError();
            fprintf(stderr, "Steam launch failed (Win32 error %lu).\n", error);
            return startup_failure("Could not open Steam. Check that Steam is installed and its steam:// links are registered.");
        }
    }

    printf("Waiting for Rust's window (up to 5 minutes). Press End to cancel.\n");
    fflush(stdout);
    const ULONGLONG startup_deadline = GetTickCount64() + 300000;
    while (!m.running(L"RustClient.exe") || !FindWindowW(L"UnityWndClass", L"Rust")) {
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            printf("Startup cancelled.\n");
            return EXIT_SUCCESS;
        }
        if (GetTickCount64() >= startup_deadline) {
            return startup_failure("Rust did not become ready within 5 minutes. Check Steam for sign-in, updates, or launch prompts.");
        }
        Sleep(500);
    }

    if (!m.attach(L"RustClient.exe")) {
        return startup_failure("Could not attach to RustClient.exe. See the memory-backend diagnostic above for the failed step and error details.");
    }

    globals::game_assembly = m.get_module_base(L"GameAssembly.dll");
    if (!globals::game_assembly) {
        m.detach();
        return startup_failure("GameAssembly.dll was not found. The game may still be loading, or the memory backend failed.");
    }

    if (!overlay::initialize(L"UnityWndClass", L"Rust")) {
        m.detach();
        return startup_failure("Overlay initialization failed. See the diagnostic above.");
    }

    esp::start_update_thread();

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        if (!overlay::scale()) {
            fprintf(stderr, "The target window is no longer available. Closing overlay.\n");
            break;
        }

        if (GetAsyncKeyState(VK_END) & 0x8000)
            break;

        auto shown = GetKeyState(VK_INSERT);
        overlay::click_through(!shown);

        misc::update();
        silent_aim::update();

        window::new_frame();

        // FPS thingy

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImVec2 text_pos = ImVec2(52.0f, 52.0f);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f FPS", ImGui::GetIO().Framerate);
        ImVec2 text_size = ImGui::CalcTextSize(buf);
        float pad_x = 8.0f;
        float pad_y = 4.0f;
        ImVec2 rect_min = ImVec2(text_pos.x - pad_x, text_pos.y - pad_y);
        ImVec2 rect_max = ImVec2(text_pos.x + text_size.x + pad_x,
            text_pos.y + text_size.y + pad_y);
        ImU32 bg_color = IM_COL32(0, 0, 0, 140);
        float rounding = 6.0f;
        draw_list->AddRectFilled(rect_min, rect_max, bg_color, rounding);
        draw_list->AddText(ImVec2(text_pos.x + 1.0f, text_pos.y + 1.0f), IM_COL32(0, 0, 0, 220), buf);
        draw_list->AddText(text_pos, ImColor(100, 220, 255, 255), buf);

        esp::render();
        silent_aim::render();
        overlay::render_crosshair();

        if (shown) {
            static int active_tab = 0;
            ImGui::SetNextWindowSize(ImVec2(600, 440), ImGuiCond_Once);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

            if (ImGui::Begin("Visual Cheats", nullptr, flags)) {
                ImGui::BeginGroup();
                ImGui::SetCursorPos(ImVec2(20, 15));
                ImGui::TextDisabled("Visual Cheats");

                ImGui::SameLine(ImGui::GetWindowWidth() - 225);
                ui::tab_button("Aimbot", 0, &active_tab); ImGui::SameLine();
                ui::tab_button("Visuals", 1, &active_tab); ImGui::SameLine();
                ui::tab_button("Misc", 2, &active_tab); ImGui::SameLine();
                ui::tab_button("Settings", 3, &active_tab);
                ImGui::EndGroup();
                ImGui::Spacing();

                ImVec2 win_pos = ImGui::GetWindowPos();
                ImVec2 win_size = ImGui::GetWindowSize();

                float thickness = 2.0f;

                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(win_pos.x, ImGui::GetCursorScreenPos().y),
                    ImVec2(win_pos.x + win_size.x, ImGui::GetCursorScreenPos().y + thickness),
                    IM_COL32(77, 168, 255, 255)
                );

                ImGui::Dummy(ImVec2(win_size.x, thickness));

                ImGui::Dummy(ImVec2(0, 5));
                ImGui::SetCursorPosX(20);

                if (active_tab == 0) {

                    ui::child_with_bar("AimMain", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("General"); ImGui::Spacing();
                    ui::checkbox("Enable Aimbot", &silent_aim::enabled);
                    ui::checkbox("Silent Aim", &silent_aim::silentaim);
                    ui::checkbox("Draw FOV", &silent_aim::draw_fov);
                    ui::checkbox("Target Sleepers", &silent_aim::target_sleepers);
                    ui::checkbox("Target NPCs", &silent_aim::target_npcs);
                    ImGui::EndChild();

                    ImGui::SameLine(305);

                    ui::child_with_bar("AimSettings", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Settings");

                    ImGui::Spacing();
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 3));
                    ImGui::PushItemWidth(245);

                    ImGui::SetCursorPosX(15);
                    ImGui::TextDisabled("Field of View");
                    ImGui::SetCursorPosX(15);
                    ImGui::SliderFloat("##FOV", &silent_aim::fov, 10, 1000, "%.0f");

                    ImGui::PopItemWidth();
                    ImGui::PopStyleVar();
                    ImGui::EndChild();
                }
                else if (active_tab == 1) {

                    ui::child_with_bar("VisMain", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Visuals"); ImGui::Spacing();
                    ui::checkbox("Enable", &esp::enabled);

                    const char* box_types[] = { "Box", "Corner Box" };
                    static int box_style = 0;

                    ImGui::SetCursorPosX(15);
                    ImGui::TextDisabled("Box Type");
                    ImGui::SetCursorPosX(15);
                    ImGui::PushItemWidth(120);
                    if (ImGui::Combo("##BoxType", &box_style, box_types, IM_ARRAYSIZE(box_types))) {
                        esp::draw_boxes = (box_style == 0);
                        esp::draw_corner_box = (box_style == 1);
                    }
                    ImGui::PopItemWidth();

                    if (box_style >= 0) {
                        ImGui::SameLine();
                        ImGui::ColorEdit3("##BoxColor", (float*)&esp::col_npc, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    }

                    ui::checkbox_color("Box Fill", &esp::draw_box_fill, esp::col_box_fill);
                    ui::checkbox_color("Name", &esp::draw_names, esp::col_name);
                    ui::checkbox_color("Distance", &esp::draw_distance, esp::col_distance);
                    ui::checkbox_color("Snaplines", &esp::draw_snaplines, esp::col_snapline);

                    ImGui::SetCursorPosX(15);
                    ImGui::TextDisabled("Max Distance");
                    ImGui::SetCursorPosX(15);
                    ImGui::SliderFloat("##Max Distance", &esp::max_distance, 50.f, 300.f, "%.0fm");

                    ui::checkbox("Custom Crosshair", &esp::crosshair_enabled);

                    if (esp::crosshair_enabled) {
                        ImGui::SetCursorPosX(15);
                        ImGui::ColorEdit3("##CrosshairColor", esp::crosshair_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                        ImGui::SameLine();
                        ImGui::Text("Color");

                        ImGui::SetCursorPosX(15);
                        ImGui::TextDisabled("Size");
                        ImGui::SetCursorPosX(15);
                        ImGui::SliderFloat("##CrosshairSize", &esp::crosshair_normal_size, 1.0f, 50.0f, "%.0f");

                        ImGui::SetCursorPosX(15);
                        ImGui::TextDisabled("Gap");
                        ImGui::SetCursorPosX(15);
                        ImGui::SliderFloat("##CrosshairGap", &esp::crosshair_normal_gap, 0.0f, 50.0f, "%.0f");

                        ImGui::SetCursorPosX(15);
                        ImGui::TextDisabled("Gap Tween");
                        ImGui::SetCursorPosX(15);
                        ImGui::SliderFloat("##CrosshairGapTween", &esp::crosshair_normal_gapTween, 0.0f, 1.0f, "%.0f");

                        if (esp::crosshair_normal_gapTween > 0.0f) {
                            ImGui::SetCursorPosX(15);
                            ImGui::TextDisabled("Gap Speed");
                            ImGui::SetCursorPosX(15);
                            ImGui::SliderFloat("##CrosshairGapSpeed", &esp::crosshair_normal_gapSpeed, 0.1f, 10.0f, "%.1f");
                        }

                        ImGui::SetCursorPosX(15);
                        ImGui::TextDisabled("Thickness");
                        ImGui::SetCursorPosX(15);
                        ImGui::SliderFloat("##CrosshairThickness", &esp::crosshair_normal_thickness, 0.5f, 10.0f, "%.1f");

                        ImGui::SetCursorPosX(15);
                        ImGui::TextDisabled("Style");
                        ImGui::SetCursorPosX(15);
                        ImGui::PushItemWidth(120);
                        const char* crosshair_styles[] = { "Static", "Pulse" };
                        ImGui::Combo("##CrosshairStyle", &esp::crosshair_normal_styleIdx, crosshair_styles, IM_ARRAYSIZE(crosshair_styles));
                        ImGui::PopItemWidth();

                        ImGui::SetCursorPosX(15);
                        ImGui::TextDisabled("Base Speed");
                        ImGui::SetCursorPosX(15);
                        ImGui::SliderFloat("##CrosshairBaseSpeed", &esp::crosshair_normal_baseSpeed, 1.0f, 500.0f, "%.0f");

                        ImGui::SetCursorPosX(15);
                        ImGui::TextDisabled("Fade Duration");
                        ImGui::SetCursorPosX(15);
                        ImGui::SliderFloat("##CrosshairFadeDuration", &esp::crosshair_normal_fadeDuration, 0.1f, 5.0f, "%.1f");
                    }

                    ImGui::EndChild();

                    ImGui::SameLine(305);

                    ui::child_with_bar("VisOptions", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Options"); ImGui::Spacing();
                    ui::checkbox("Show Sleepers", &esp::show_sleepers);
                    ui::checkbox("Show Wounded", &esp::show_wounded);

                    ImGui::Spacing();
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Entity Colors"); ImGui::Spacing();
                    ui::color_row("Team", esp::col_team);
                    ui::color_row("Sleeper", esp::col_sleeper);
                    ui::color_row("Wounded", esp::col_wounded);
                    ImGui::EndChild();
                }
                else if (active_tab == 2) {

                    ui::child_with_bar("MiscLeft", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Misc"); ImGui::Spacing();
                    ui::checkbox("Bright Night", &misc::bright_night);
                    ui::checkbox("Spiderman", &misc::spider_man);
                    ui::checkbox("Fake Admin", &misc::admin_flags);
                    ui::checkbox("Third Person", &misc::third_person);
                    ImGui::EndChild();

                    ImGui::SameLine(305);

                    ui::child_with_bar("MiscRight", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Extra"); ImGui::Spacing();
                    ImGui::EndChild();
                }
                else if (active_tab == 3) {

                    ui::child_with_bar("CfgLeft", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Config"); ImGui::Spacing();
                    ImGui::SetCursorPosX(15);
                    if (ImGui::Button("Save Config", ImVec2(245, 35))) {}
                    ImGui::Spacing(); ImGui::SetCursorPosX(15);
                    if (ImGui::Button("Load Config", ImVec2(245, 35))) {}
                    ImGui::EndChild();

                    ImGui::SameLine(305);

                    ui::child_with_bar("CfgRight", ImVec2(275, 355));
                    ImGui::SetCursorPosX(10); ImGui::TextDisabled("Info"); ImGui::Spacing();
                    ImGui::SetCursorPosX(15); ImGui::TextDisabled("Build: Feb 23 2026");
                    ImGui::SetCursorPosX(15); ImGui::TextDisabled("Status: Undetected");
                    ImGui::EndChild();
                }
            }
            ImGui::End();
        }

        window::draw();
    }

    esp::stop_update_thread();
    window::cleanup();
    m.detach();

    return 0;
}
