#pragma once
#include <Windows.h>
#include <cstdint>

// ウィンドウの生成とメッセージ処理を担当するクラス
class WinApp {
public:
	// クライアント領域のサイズ
	static constexpr int32_t kClientWidth = 1280;
	static constexpr int32_t kClientHeight = 720;

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void Initialize();
	void Finalize();
	// メッセージ処理。WM_QUIT(×ボタン)が来たらtrueを返す
	bool ProcessMessage();

	HWND GetHwnd() const { return hwnd_; }
	HINSTANCE GetHInstance() const { return wc_.hInstance; }

private:
	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};
};
