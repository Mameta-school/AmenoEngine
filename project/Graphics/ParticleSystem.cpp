#include "ParticleSystem.h"
#include "ModelLoader.h"
#include <cassert>
#include <cmath>

void ParticleSystem::Initialize(DirectXCommon* dxCommon, ShaderCompiler* shaderCompiler, uint32_t srvIndex) {
	assert(dxCommon);
	dxCommon_ = dxCommon;
	ID3D12Device* device = dxCommon_->GetDevice();

	// Particle用のRootSignature
	pipeline_.Initialize(device, shaderCompiler);

	// 板ポリの頂点バッファ
	ModelData modelData = CreatePlaneModel();
	vertexBuffer_ = CreateVertexBuffer(device, modelData.vertices);

	// Instancing用リソース
	instancingResource_ = CreateBufferResource(device, sizeof(ParticleForGPU) * kNumMaxInstance);
	instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));
	for (uint32_t i = 0; i < kNumMaxInstance; ++i) {
		instancingData_[i].WVP = MakeIdentity4x4();
		instancingData_[i].World = MakeIdentity4x4();
		instancingData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	}

	// StructuredBuffer用のSRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = kNumMaxInstance;
	srvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	device->CreateShaderResourceView(instancingResource_.Get(), &srvDesc, dxCommon_->GetSrvCPUHandle(srvIndex));
	instancingSrvHandleGPU_ = dxCommon_->GetSrvGPUHandle(srvIndex);

	// 乱数の準備と、粒の初期化
	std::random_device seedGenerator;
	randomEngine_.seed(seedGenerator());
	for (uint32_t i = 0; i < kNumMaxInstance; ++i) {
		particles_[i] = MakeParticle();
	}
}

void ParticleSystem::Update(const Matrix4x4& view, const Matrix4x4& proj) {
	const float kDeltaTime = 1.0f / 60.0f;	// FPS固定
	Matrix4x4 viewProjectionMatrix = Multiply(view, proj);

	for (uint32_t i = 0; i < kNumMaxInstance; ++i) {
		Particle& p = particles_[i];

		// 速度 × 時間だけ動かす
		p.transform.translate.x += p.velocity.x * kDeltaTime;
		p.transform.translate.y += p.velocity.y * kDeltaTime;
		p.transform.translate.z += p.velocity.z * kDeltaTime;

		// 遠くまで行ったら、中心から作り直す
		const Vector3& t = p.transform.translate;
		if (t.x * t.x + t.y * t.y > 9.0f) {
			p = MakeParticle();
		}

		// GPU用バッファへ書き込む
		Matrix4x4 world = MakeAffineMatrix(p.transform.scale, p.transform.rotate, p.transform.translate);
		instancingData_[i].WVP = Multiply(world, viewProjectionMatrix);
		instancingData_[i].World = world;
		instancingData_[i].color = p.color;
	}
}

void ParticleSystem::Draw(
	BlendMode blendMode,
	D3D12_GPU_VIRTUAL_ADDRESS materialAddress,
	D3D12_GPU_VIRTUAL_ADDRESS lightAddress,
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// RootSignatureが切り替わるので、毎回設定し直す
	pipeline_.SetPipeline(commandList, blendMode);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootConstantBufferView(kRootParamMaterial, materialAddress);
	commandList->SetGraphicsRootConstantBufferView(kRootParamDirectionalLight, lightAddress);
	commandList->SetGraphicsRootDescriptorTable(kRootParamTexture, textureHandle);
	commandList->SetGraphicsRootDescriptorTable(kRootParamTransformation, instancingSrvHandleGPU_);

	commandList->IASetVertexBuffers(0, 1, &vertexBuffer_.view);
	commandList->DrawInstanced(vertexBuffer_.vertexCount, kNumMaxInstance, 0, 0);
}

ParticleSystem::Particle ParticleSystem::MakeParticle() {
	std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * kPi);
	std::uniform_real_distribution<float> distSpeed(0.5f, 0.5f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);

	float angle = distAngle(randomEngine_);
	float speed = distSpeed(randomEngine_);

	Particle particle;
	particle.transform.scale = { 0.2f, 0.2f, 0.2f };
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.transform.translate = { 0.0f, 0.0f, 0.0f };
	particle.velocity = { std::cos(angle) * speed, std::sin(angle) * speed, 0.0f };
	particle.color = { distColor(randomEngine_), distColor(randomEngine_), distColor(randomEngine_), 1.0f };
	return particle;
}