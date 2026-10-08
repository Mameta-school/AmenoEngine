#include "ImGuiManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] DirectXCommon* dxCommon) {
#ifdef USE_IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	// SRVヒープの先頭(index 0)をImGuiが使う
	ImGui_ImplDX12_Init(
		dxCommon->GetDevice(),
		DirectXCommon::kBackBufferCount,
		DirectXCommon::kRtvFormat,
		dxCommon->GetSrvDescriptorHeap(),
		dxCommon->GetSrvCPUHandle(0),
		dxCommon->GetSrvGPUHandle(0));
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();
#endif
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	// 初期化と逆順に行う
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif
}

void ImGuiManager::End() {
#ifdef USE_IMGUI
	// ImGuiの内部コマンドを生成する
	ImGui::Render();
#endif
}

void ImGuiManager::Draw([[maybe_unused]] ID3D12GraphicsCommandList* commandList) {
#ifdef USE_IMGUI
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif
}
