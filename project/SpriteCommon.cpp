#include "SpriteCommon.h"
#include "GraphicsPipeline.h"
#include "Logger.h"
#include <cassert>

using Microsoft::WRL::ComPtr;

void SpriteCommon::Initialize(DirectXCommon* dxCommon, ShaderCompiler* shaderCompiler) {
	assert(dxCommon);
	dxCommon_ = dxCommon;

	// パイプライン生成
	CreateGraphicsPipeline(shaderCompiler);
}

void SpriteCommon::CommonDrawSetting() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void SpriteCommon::CreateRootSignature() {
	// GraphicsPipeline::CreateRootSignature の中身をそのままコピーする。
	// 違うのは、device を dxCommon_->GetDevice() で取るところと、
	// 生成先が rootSignature_ になるところだけ
	ID3D12Device* device = dxCommon_->GetDevice();

	// ... (GraphicsPipeline.cpp の CreateRootSignature の本体)
}

void SpriteCommon::CreateGraphicsPipeline(ShaderCompiler* shaderCompiler) {
	// 最初にルートシグネチャを作る(PSOの生成に必要なため)
	CreateRootSignature();

	ID3D12Device* device = dxCommon_->GetDevice();

	// GraphicsPipeline::CreatePipelineStates の本体をコピーして、次の点だけ変える
	// ... InputLayout、Shaderのコンパイル、desc の設定

	// ブレンドは「通常(αブレンド)」の1つだけ作る(ループは不要)
	// 例: desc.BlendState = ... Normal の設定 ...
	// HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	// assert(SUCCEEDED(hr));
}