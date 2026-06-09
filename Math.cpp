#include "Math.h"
#include <cmath>

Matrix4x4 MakeIdentity4x4() {
	Matrix4x4 result{};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 result{};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				result.m[i][j] += a.m[i][k] * b.m[k][j];
	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 rotateX{};
	rotateX.m[0][0] = 1.0f;
	rotateX.m[1][1] = std::cos(rotate.x);
	rotateX.m[1][2] = std::sin(rotate.x);
	rotateX.m[2][1] = -std::sin(rotate.x);
	rotateX.m[2][2] = std::cos(rotate.x);
	rotateX.m[3][3] = 1.0f;

	Matrix4x4 rotateY{};
	rotateY.m[0][0] = std::cos(rotate.y);
	rotateY.m[0][2] = -std::sin(rotate.y);
	rotateY.m[1][1] = 1.0f;
	rotateY.m[2][0] = std::sin(rotate.y);
	rotateY.m[2][2] = std::cos(rotate.y);
	rotateY.m[3][3] = 1.0f;

	Matrix4x4 rotateZ{};
	rotateZ.m[0][0] = std::cos(rotate.z);
	rotateZ.m[0][1] = std::sin(rotate.z);
	rotateZ.m[1][0] = -std::sin(rotate.z);
	rotateZ.m[1][1] = std::cos(rotate.z);
	rotateZ.m[2][2] = 1.0f;
	rotateZ.m[3][3] = 1.0f;

	Matrix4x4 rotateXYZ = Multiply(Multiply(rotateX, rotateY), rotateZ);

	Matrix4x4 result{};
	result.m[0][0] = scale.x * rotateXYZ.m[0][0];
	result.m[0][1] = scale.x * rotateXYZ.m[0][1];
	result.m[0][2] = scale.x * rotateXYZ.m[0][2];
	result.m[1][0] = scale.y * rotateXYZ.m[1][0];
	result.m[1][1] = scale.y * rotateXYZ.m[1][1];
	result.m[1][2] = scale.y * rotateXYZ.m[1][2];
	result.m[2][0] = scale.z * rotateXYZ.m[2][0];
	result.m[2][1] = scale.z * rotateXYZ.m[2][1];
	result.m[2][2] = scale.z * rotateXYZ.m[2][2];
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 Inverse(const Matrix4x4& m) {
	Matrix4x4 result{};
	float det =
		m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]) -
		m.m[0][1] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]) +
		m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]);
	float invDet = 1.0f / det;
	result.m[0][0] = invDet * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]);
	result.m[0][1] = -invDet * (m.m[0][1] * m.m[2][2] - m.m[0][2] * m.m[2][1]);
	result.m[0][2] = invDet * (m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1]);
	result.m[1][0] = -invDet * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]);
	result.m[1][1] = invDet * (m.m[0][0] * m.m[2][2] - m.m[0][2] * m.m[2][0]);
	result.m[1][2] = -invDet * (m.m[0][0] * m.m[1][2] - m.m[0][2] * m.m[1][0]);
	result.m[2][0] = invDet * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]);
	result.m[2][1] = -invDet * (m.m[0][0] * m.m[2][1] - m.m[0][1] * m.m[2][0]);
	result.m[2][2] = invDet * (m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0]);
	result.m[3][0] = -(m.m[3][0] * result.m[0][0] + m.m[3][1] * result.m[1][0] + m.m[3][2] * result.m[2][0]);
	result.m[3][1] = -(m.m[3][0] * result.m[0][1] + m.m[3][1] * result.m[1][1] + m.m[3][2] * result.m[2][1]);
	result.m[3][2] = -(m.m[3][0] * result.m[0][2] + m.m[3][1] * result.m[1][2] + m.m[3][2] * result.m[2][2]);
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	Matrix4x4 result{};
	float tanHalfFov = std::tan(fovY / 2.0f);
	result.m[0][0] = 1.0f / (aspectRatio * tanHalfFov);
	result.m[1][1] = 1.0f / tanHalfFov;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = -nearClip * farClip / (farClip - nearClip);
	return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 result{};
	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[3][0] = -(right + left) / (right - left);
	result.m[3][1] = -(top + bottom) / (top - bottom);
	result.m[3][2] = -nearClip / (farClip - nearClip);
	result.m[3][3] = 1.0f;
	return result;
}