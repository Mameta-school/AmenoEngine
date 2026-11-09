#include "DebugCamera.h"
#include <dinput.h>

void DebugCamera::Initialize(float aspectRatio) {
	// 射影行列の生成
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, aspectRatio, 0.1f, 1000.0f);

	// 累積回転行列を単位行列で初期化
	matRot_ = MakeIdentity4x4();

	// ビュー行列の初期生成
	Matrix4x4 translateMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, translation_);
	Matrix4x4 cameraMatrix = Multiply(matRot_, translateMatrix);
	viewMatrix_ = Inverse(cameraMatrix);
}

void DebugCamera::Update(const unsigned char* key) {
	// 回転操作
	const float rotateSpeed = 0.02f;
	// フレームで加える回転量
	Vector3 rotateAmount = { 0.0f, 0.0f, 0.0f };

	if (key[DIK_UP]) {
		rotateAmount.x -= rotateSpeed;
	}

	if (key[DIK_DOWN]) {
		rotateAmount.x += rotateSpeed;
	}

	if (key[DIK_LEFT]) {
		rotateAmount.y -= rotateSpeed;
	}

	if (key[DIK_RIGHT]) {
		rotateAmount.y += rotateSpeed;
	}

	Matrix4x4 matRotDelta = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotateAmount, { 0.0f, 0.0f, 0.0f });

	// 累積回転行列の更新
	matRot_ = Multiply(matRotDelta, matRot_);

	// 移動操作
	const float moveSpeed = 0.5f;
	Vector3 move = { 0.0f, 0.0f, 0.0f };

	if (key[DIK_W]) {
		move.z += moveSpeed;
	}

	if (key[DIK_S]) {
		move.z -= moveSpeed;
	}

	if (key[DIK_A]) {
		move.x -= moveSpeed;
	}

	if (key[DIK_D]) {
		move.x += moveSpeed;
	}

	if (key[DIK_Q]) {
		move.y -= moveSpeed;
	}

	if (key[DIK_E]) {
		move.y += moveSpeed;
	}

	// 移動量をカメラが向いている方向に変換する
	Vector3 worldMove{};
	worldMove.x = move.x * matRot_.m[0][0] + move.y * matRot_.m[1][0] + move.z * matRot_.m[2][0];
	worldMove.y = move.x * matRot_.m[0][1] + move.y * matRot_.m[1][1] + move.z * matRot_.m[2][1];
	worldMove.z = move.x * matRot_.m[0][2] + move.y * matRot_.m[1][2] + move.z * matRot_.m[2][2];

	translation_.x += worldMove.x;
	translation_.y += worldMove.y;
	translation_.z += worldMove.z;

	// ビュー行列の更新
	Matrix4x4 translateMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, translation_);
	Matrix4x4 cameraMatrix = Multiply(matRot_, translateMatrix);
	viewMatrix_ = Inverse(cameraMatrix);
}