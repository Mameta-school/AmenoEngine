#pragma once
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl.h>
#include "WinApp.h"

// キーボード入力
class Input {
public:
	void Initialize(WinApp* winApp);
	// 毎フレーム呼ぶ。キー状態を更新する
	void Update();

	// キーが押されているか
	bool PushKey(BYTE keyNumber);
	bool TriggerKey(BYTE keyNumber);
	bool ReleaseKey(BYTE keyNumber);
	// 全キーの状態
	const BYTE* GetKeys() const { return key_; }

private:
	// WinAppのポインタを借りて保持する
	WinApp* winApp_ = nullptr;

	Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
	BYTE key_[256] = {};
	BYTE keyPre_[256] = {};
};
