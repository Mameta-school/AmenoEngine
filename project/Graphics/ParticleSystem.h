#pragma once
#include <cstdint>
#include <random>
#include <wrl.h>
#include "DirectXCommon.h"
#include "GraphicsPipeline.h"
#include "ParticlePipeline.h"
#include "ResourceUtility.h"
#include "ShaderCompiler.h"
#include "MathUtility.h"

// パーティクル全体を管理するクラス
class ParticleSystem {
public:
	static constexpr uint32_t kNumMaxInstance = 30;

	// srvIndex: Instancing用SRVを作るSRVヒープの番号
	void Initialize(DirectXCommon* dxCommon, ShaderCompiler* shaderCompiler, uint32_t srvIndex);
	// 毎フレーム呼ぶ。粒を動かして、GPU用バッファに書き込む
	void Update(const Matrix4x4& view, const Matrix4x4& proj);
	// 描画する
	void Draw(
		BlendMode blendMode,
		D3D12_GPU_VIRTUAL_ADDRESS materialAddress,
		D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

private:
	// 粒1つ分のデータ
	struct Particle {
		Transform transform;
		Vector3 velocity;
		Vector4 color;
		float lifeTime;
		float currentTime;
	};
	// 粒1つ分のデータ
	struct ParticleForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	// 中心から、ランダムな向き・速さ・色で粒を作る
	Particle MakeParticle();

	DirectXCommon* dxCommon_ = nullptr;

	ParticlePipeline pipeline_;
	VertexBuffer vertexBuffer_;
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
	ParticleForGPU* instancingData_ = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_{};

	Particle particles_[kNumMaxInstance];
	uint32_t numInstance_ = 0;
	std::mt19937 randomEngine_;
};