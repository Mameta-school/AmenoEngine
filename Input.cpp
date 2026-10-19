#include "Input.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
	// DirectInputの初期化
	HRESULT hr = DirectInput8Create(
		hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		reinterpret_cast<void**>(directInput_.GetAddressOf()), nullptr);
	assert(SUCCEEDED(hr));

	// キーボードデバイスの生成
	hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, nullptr);
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = keyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));
}

void Input::Update() {
	// 前回のキー入力を保存
	memcpy(keyPre_, key_, sizeof(key_));
	// キーボード情報の取得開始
	keyboard_->Acquire();
	keyboard_->GetDeviceState(sizeof(key_), key_);
}

bool Input::PushKey(BYTE keyNumber) {
	// 指定キーを推してtrueを返す
	if (key_[keyNumber]) {
		return true;
	}
	// そうでなければfalseを返す
	return false;
}

bool Input::TriggerKey(BYTE keyNumber) {
	// 今フレームは推されていていればtrueを返す
	if (key_[keyNumber] && !keyPre_[keyNumber]) {
		return true;
	}
	// そうでなければfalseを返す
	return false;
}
