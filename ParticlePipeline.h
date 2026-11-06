#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "ShaderCompiler.h"
#include "GraphicsPipeline.h"

class ParticlePipeline {
public:
	void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);
	void SetPipeline(ID3D12GraphicsCommandList* commandList, BlendMode blendMode) const;

private:
	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineStates(ID3D12Device* device, ShaderCompiler* shaderCompiler);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStates_[kCountOfBlendMode];
};