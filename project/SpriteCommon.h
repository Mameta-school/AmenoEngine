#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "DirectXCommon.h"
#include "ShaderCompiler.h"

class SpriteCommon {
public:
	void Initialize(DirectXCommon* dxCommon, ShaderCompiler* shaderCompiler);
	// 共通描画設定
	void CommonDrawSetting();

	DirectXCommon* GetDxCommon() const { return dxCommon_; }

private:
	// ルートシグネチャの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void CreateGraphicsPipeline(ShaderCompiler* shaderCompiler);

	DirectXCommon* dxCommon_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};