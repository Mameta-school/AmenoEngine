#pragma once
#include <d3d12.h>
#include "WinApp.h"
#include "DirectXCommon.h"

// ImGuiの初期化・フレーム開始/終了・描画をまとめたクラス
// USE_IMGUI が定義されていないときは、すべて何もしない
class ImGuiManager {
public:
	void Initialize(WinApp* winApp, DirectXCommon* dxCommon);
	void Finalize();
	// フレーム開始(ImGui::NewFrame)。ImGuiのウィジェットを書く前に呼ぶ
	void Begin();
	// フレーム終了(ImGui::Render)。ウィジェットを書き終わったら呼ぶ
	void End();
	// 描画コマンドを積む。DirectXCommon::PostDraw の前に呼ぶ
	void Draw(ID3D12GraphicsCommandList* commandList);
};
