#pragma once
#include <cstdint>

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
struct Vector4 { float x, y, z, w; };
struct Matrix4x4 { float m[4][4]; };
struct Transform { Vector3 scale, rotate, translate; };
struct VertexData { Vector4 position; Vector2 texcoord; Vector3 normal; };
struct Material { Vector4 color; int32_t enableLighting; float padding[3]; Matrix4x4 uvTransform; };
struct TransformationMatrix { Matrix4x4 WVP; Matrix4x4 World; };
struct DirectionalLight { Vector4 color; Vector3 direction; float intensity; };

Matrix4x4 MakeIdentity4x4();
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
Matrix4x4 Inverse(const Matrix4x4& m);
Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b);
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);