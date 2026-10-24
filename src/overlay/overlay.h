#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <cstdint>
#include "../../ext/imgui/imgui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace window {
	inline HWND hwnd;
	inline HINSTANCE instance;
	inline uint32_t width, height;

	namespace directx
	{
		inline ID3D11Device* device = nullptr;
		inline ID3D11DeviceContext* context = nullptr;
		inline IDXGISwapChain* swap_chain = nullptr;
		inline ID3D11RenderTargetView* render_target_view = nullptr;
	}
	
	LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	void cleanup();
	bool create_window();
	bool create_device();
	void new_frame();
	void draw();
}

namespace overlay {
	inline HWND target;
	inline uint32_t width, height;


	bool initialize(const wchar_t* window_class, const wchar_t* window_name);
	bool scale();
	void click_through(bool click);
	void render_crosshair();
}

namespace ui {
	void apply_saku_style();
	void tab_button(const char* label, int index, int* active_tab);
	bool checkbox(const char* label, bool* v);
	bool checkbox_color(const char* label, bool* v, float* col);
	void color_row(const char* label, float* col);
	void child_with_bar(const char* id, ImVec2 size);
}
