#include "overlay.h"
#include "../bytes/font.h"
#include "../features/esp/esp.h"
#define STB_IMAGE_IMPLEMENTATION
#include <dwmapi.h>
#include <ctime>
#include <algorithm>
#include <cmath>
#include "../../ext/imgui/imgui_impl_dx11.h"
#include "../../ext/imgui/imgui_impl_win32.h"





LRESULT WINAPI window::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
        return true;


    switch (Msg)
    {
    case WM_SIZE:
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}


bool window::create_device() {
    if (!hwnd || width <= 0 || height <= 0)
        return false;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window::hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;               
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    UINT create_device_flags = 0;
    D3D_FEATURE_LEVEL feature_level;

    directx::swap_chain = nullptr;
    directx::device = nullptr;
    directx::context = nullptr;

    IDXGIFactory* dxgi_factory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgi_factory);
    if (FAILED(hr))
        return false;

    IDXGIAdapter* selected_adapter = nullptr;
    SIZE_T max_dedicated_video_memory = 0;
    IDXGIAdapter* adapter = nullptr;
    UINT i = 0;

    while (dxgi_factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC adapter_desc;
        adapter->GetDesc(&adapter_desc);
        if (adapter_desc.DedicatedVideoMemory > max_dedicated_video_memory) {
            max_dedicated_video_memory = adapter_desc.DedicatedVideoMemory;
            if (selected_adapter)
                selected_adapter->Release();
            selected_adapter = adapter;
        }
        else {
            adapter->Release();
        }
        ++i;
    }

    if (selected_adapter) {
        hr = D3D11CreateDeviceAndSwapChain(
            selected_adapter,
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            create_device_flags,
            featureLevelArray,
            _countof(featureLevelArray),
            D3D11_SDK_VERSION,
            &sd,
            &directx::swap_chain,
            &directx::device,
            &feature_level,
            &directx::context
        );
        selected_adapter->Release();
    }
    else {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            create_device_flags,
            featureLevelArray,
            _countof(featureLevelArray),
            D3D11_SDK_VERSION,
            &sd,
            &directx::swap_chain,
            &directx::device,
            &feature_level,
            &directx::context
        );
    }

    dxgi_factory->Release();

    if (FAILED(hr)) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            create_device_flags,
            featureLevelArray,
            _countof(featureLevelArray),
            D3D11_SDK_VERSION,
            &sd,
            &directx::swap_chain,
            &directx::device,
            &feature_level,
            &directx::context
        );
        if (FAILED(hr)) {
            fprintf(stderr, "Direct3D device creation failed (HRESULT 0x%08lX).\n", static_cast<unsigned long>(hr));
            return false;
        }
    }

    if (!directx::swap_chain || !directx::device || !directx::context)
        return false;

    ID3D11Texture2D* back_buffer = nullptr;
    hr = directx::swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (FAILED(hr) || !back_buffer)
        return false;

    hr = directx::device->CreateRenderTargetView(back_buffer, nullptr, &directx::render_target_view);
    back_buffer->Release();
    back_buffer = nullptr;
    if (FAILED(hr) || !directx::render_target_view)
        return false;

    directx::context->OMSetRenderTargets(1, &directx::render_target_view, nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    directx::context->RSSetViewports(1, &viewport);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    io.Fonts->AddFontFromMemoryTTF((void*)font_bytes, sizeof(font_bytes), 16.0f);

    ui::apply_saku_style();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(directx::device, directx::context);

    ImGui_ImplDX11_CreateDeviceObjects();

    return true;
}

void window::new_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void window::draw() {
    ImGui::EndFrame();
    ImGui::Render();

    static constexpr float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
    directx::context->ClearRenderTargetView(directx::render_target_view, clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present(0, 0) = no vsync, snappier response
    directx::swap_chain->Present(0, 0);
}

void window::cleanup() {
    if (window::hwnd) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (directx::render_target_view) { directx::render_target_view->Release(); directx::render_target_view = nullptr; }
        if (directx::swap_chain) { directx::swap_chain->Release(); directx::swap_chain = nullptr; }
        if (directx::context) { directx::context->Release(); directx::context = nullptr; }
        if (directx::device) { directx::device->Release(); directx::device = nullptr; }
        DestroyWindow(hwnd);
    }
}

bool window::create_window() {
    auto random_string = [](size_t length) -> const wchar_t* {
        const wchar_t charset[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        const size_t charsetSize = wcslen(charset);
        wchar_t* randomString = new wchar_t[length + 1];
        static bool seeded = false;
        if (!seeded) {
            srand(static_cast<unsigned int>(time(nullptr)));
            seeded = true;
        }
        for (size_t i = 0; i < length; ++i) {
            randomString[i] = charset[rand() % charsetSize];
        }
        randomString[length] = L'\0';
        return randomString;
        };

    auto window_name = random_string(16);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window::WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = window_name;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    if (!RegisterClassEx(&wc))
        return false;

    window::width = GetSystemMetrics(SM_CXSCREEN);
    window::height = GetSystemMetrics(SM_CYSCREEN);

    window::hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        window_name,
        random_string(16),
        WS_POPUP,
        0, 0, window::width, window::height,
        nullptr, nullptr,
        wc.hInstance,
        nullptr
    );

    if (!window::hwnd) {
        UnregisterClass(window_name, wc.hInstance);
        return false;
    }

    MARGINS margin = { -1, -1, -1, -1 };
    HRESULT hr = DwmExtendFrameIntoClientArea(window::hwnd, &margin);
    if (FAILED(hr)) {
        UnregisterClass(window_name, wc.hInstance);
        DestroyWindow(window::hwnd);
        window::hwnd = nullptr;
        return false;
    }

    ShowWindow(window::hwnd, SW_SHOW);
    // UpdateWindow may return zero when there is no pending paint region.
    UpdateWindow(window::hwnd);
    return true;
}

bool overlay::scale() {
    static RECT old = {};

    HWND foreground_window = GetForegroundWindow();

    if (foreground_window == target) {
        SetWindowPos(window::hwnd, GetWindow(foreground_window, GW_HWNDPREV), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
        UpdateWindow(window::hwnd);
    }

    RECT rc = {};
    POINT xy = {};

    if (!GetClientRect(target, &rc))
        return false;

    if (!ClientToScreen(target, &xy))
        return false;

    rc.left = xy.x;
    rc.top = xy.y;

    if (rc.left != old.left || rc.right != old.right || rc.top != old.top || rc.bottom != old.bottom) {
        old = rc;

        width = rc.right;
        height = rc.bottom;

        // SWP_NOREDRAW | SWP_NOZORDER avoids unnecessary redraws on resize
        SetWindowPos(window::hwnd, nullptr, xy.x, xy.y, static_cast<int>(width), static_cast<int>(height), SWP_NOREDRAW | SWP_NOZORDER);
    }

    return true;
}

void overlay::click_through(bool click) {
    SetWindowLong(window::hwnd, GWL_EXSTYLE, click ? WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW : WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
}

bool overlay::initialize(const wchar_t* window_class, const wchar_t* window_name)
{
    if (!window_class)
        return false;

    overlay::target = FindWindowW(window_class, window_name);

    if (!overlay::target)
    {
        fprintf(stderr, "Target window was not found (class: %ls, title: %ls).\n", window_class, window_name ? window_name : L"<any>");
        overlay::target = 0;
        return false;
    }

    if (!window::create_window()) {
        fprintf(stderr, "Overlay window creation failed (Win32 error %lu).\n", GetLastError());
        return false;
    }

    if (!window::create_device()) {
        fprintf(stderr, "Overlay graphics initialization failed.\n");
        return false;
    }

    return true;
}

void overlay::render_crosshair() {
	if (!esp::crosshair_enabled)
		return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	if (!draw)
		return;

	{
		static float fadeParam = 0.0f;
		static int lastStyle = esp::crosshair_normal_styleIdx;
		static float currentSpeed = 0.0f;
		static float angle = 0.0f;
		static ImVec2 lastPos = ImVec2(0, 0);

		float time = ImGui::GetTime();
		float dt = ImGui::GetIO().DeltaTime;

		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient((HWND)ImGui::GetMainViewport()->PlatformHandleRaw, &pt);
		ImVec2 mpos{ (float)pt.x, (float)pt.y };

		float lerpSpeed = 5.0f;
		mpos.x = lastPos.x + (mpos.x - lastPos.x) * std::clamp(lerpSpeed * dt, 0.0f, 1.0f);
		mpos.y = lastPos.y + (mpos.y - lastPos.y) * std::clamp(lerpSpeed * dt, 0.0f, 1.0f);
		lastPos = mpos;

		if (esp::crosshair_normal_styleIdx != lastStyle) {
			fadeParam = 0.0f;
			lastStyle = esp::crosshair_normal_styleIdx;
		}

		float targetSpeed = esp::crosshair_normal_baseSpeed * 0.5f;
		switch (esp::crosshair_normal_styleIdx) {
		case 0:
			targetSpeed = esp::crosshair_normal_baseSpeed * 0.5f;
			fadeParam = 0.0f;
			break;
		case 1:
			fadeParam = std::clamp(fadeParam + dt / esp::crosshair_normal_fadeDuration, 0.0f, 1.0f);
			targetSpeed = esp::crosshair_normal_baseSpeed * (0.1f + 0.9f * (fadeParam * fadeParam));
			break;
		}

		const float accel = 5.0f;
		currentSpeed += (targetSpeed - currentSpeed) * std::clamp(accel * dt, 0.0f, 1.0f);
		angle += currentSpeed * (3.1415926f / 180.0f) * dt;
		if (angle > 6.2831853f) angle -= 6.2831853f;

		float showGap = esp::crosshair_normal_gap;
		if (esp::crosshair_normal_gapTween > 0.0f) {
			float raw = fmodf(time * esp::crosshair_normal_gapSpeed, 2.0f);
			float e = raw < 1.0f ? (1.0f - (1.0f - raw) * (1.0f - raw)) : 1.0f - ((raw - 1.0f) * (raw - 1.0f));
			showGap = esp::crosshair_normal_gap * (1.0f + e * esp::crosshair_normal_gapTween);
		}

		ImU32 colLine = IM_COL32(
			(int)(esp::crosshair_color[0] * 255),
			(int)(esp::crosshair_color[1] * 255),
			(int)(esp::crosshair_color[2] * 255),
			(int)(esp::crosshair_color[3] * 255)
		);
		ImU32 colOut = IM_COL32(0, 0, 0, 255);
		float thick = esp::crosshair_normal_thickness;

		for (int i = 0; i < 4; ++i) {
			float a = angle + i * 3.1415926f * 0.5f;
			ImVec2 d{ cosf(a), sinf(a) };
			ImVec2 p0{ mpos.x + d.x * showGap, mpos.y + d.y * showGap };
			ImVec2 p1{ mpos.x + d.x * (showGap + esp::crosshair_normal_size),
					mpos.y + d.y * (showGap + esp::crosshair_normal_size) };

			for (int dx = -1; dx <= 1; ++dx)
				for (int dy = -1; dy <= 1; ++dy)
					if (dx || dy)
						draw->AddLine({ p0.x + dx, p0.y + dy },
							{ p1.x + dx, p1.y + dy },
							colOut, thick);

			draw->AddLine(p0, p1, colLine, thick);
		}
	}
}


void ui::apply_saku_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 4.0f;           
    style.GrabRounding = 12.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    style.ScrollbarSize = 4.0f;
    style.ScrollbarRounding = 2.0f;


    style.GrabMinSize = 9.0f;           
    style.FramePadding = ImVec2(8, 4);   
    style.ItemSpacing = ImVec2(8, 5);   
    style.ItemInnerSpacing = ImVec2(4, 3);   

    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);


    ImVec4 sakuBlue = ImVec4(0.30f, 0.66f, 1.00f, 1.00f); 
    colors[ImGuiCol_CheckMark] = sakuBlue;
    colors[ImGuiCol_SliderGrab] = sakuBlue;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.76f, 1.00f, 1.00f); 
    colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.30f, 0.66f, 1.00f, 0.18f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.66f, 1.00f, 0.38f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.66f, 1.00f, 0.58f);

    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.66f, 1.00f, 0.25f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.66f, 1.00f, 0.35f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.66f, 1.00f, 0.45f);

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.66f, 1.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.76f, 1.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.86f, 1.00f, 1.00f);

    colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.10f, 0.95f);
}

void ui::tab_button(const char* label, int index, int* active_tab) {
    ImVec2 screen_pos = ImGui::GetCursorScreenPos();

    if (*active_tab == index)
        ImGui::TextColored(ImVec4(0.30f, 0.66f, 1.00f, 1.00f), label);
    else
        ImGui::Text(label);

    if (ImGui::IsItemClicked()) *active_tab = index;

    if (*active_tab == index) {
        ImVec2 text_size = ImGui::CalcTextSize(label);
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(screen_pos.x, screen_pos.y + text_size.y + 2),
            ImVec2(screen_pos.x + text_size.x, screen_pos.y + text_size.y + 4),
            IM_COL32(77, 168, 255, 255)
        );
    }
}

bool ui::checkbox(const char* label, bool* v) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    return ImGui::Checkbox(label, v);
}

bool ui::checkbox_color(const char* label, bool* v, float* col) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    bool changed = ImGui::Checkbox(label, v);
    ImGui::SameLine(ImGui::GetWindowWidth() - 70);
    ImGui::SetNextItemWidth(50);
    ImGui::PushID(label);
    ImGui::ColorEdit4("##col", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
    ImGui::PopID();
    return changed;
}

void ui::color_row(const char* label, float* col) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::ColorEdit3(label, col, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine();
    ImGui::Text(label);
}

void ui::child_with_bar(const char* id, ImVec2 size)
{
    ImGui::BeginChild(id, size, true);

    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size_w = ImGui::GetWindowSize();
    float thickness = 4.0f;

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + size_w.x, pos.y + thickness),
        IM_COL32(77, 168, 255, 255)
    );

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + thickness + 3);
}
