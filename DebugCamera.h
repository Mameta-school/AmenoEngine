#pragma once
#include "Math.h"

class DebugCamera
{
public:
	void Initialize(float aspectRatio);
	void Update(const unsigned char* key);

	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
private:
	// 累積回転行列
	Matrix4x4 matRot_;
	// ローカル座標
	Vector3 translation_ = { 0, 0, -10 };
	// ビュー行列
	Matrix4x4 viewMatrix_;
	// 射影行列
	Matrix4x4 projectionMatrix_;
};

