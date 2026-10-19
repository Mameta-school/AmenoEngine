#pragma once
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl.h>

// キーボード入力
class Input {
public:
	void Initialize(HINSTANCE hInstance, HWND hwnd);
	// 毎フレーム呼ぶ。キー状態を更新する
	void Update();

	// キーが押されているか
	bool PushKey(BYTE keyNumber) const { return key_[keyNumber] != 0; }
	// 全キーの状態
	const BYTE* GetKeys() const { return key_; }

private:
	Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
	BYTE key_[256] = {};
};
