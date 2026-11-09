#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "ShaderCompiler.h"

// ブレンドモード
enum BlendMode {
	kBlendModeNone,		// ブレンドなし
	kBlendModeNormal,	// 通常(αブレンド)
	kBlendModeAdd,		// 加算
	kBlendModeSubtract,	// 減算
	kBlendModeMultiply,	// 乗算
	kBlendModeScreen,	// スクリーン
	kCountOfBlendMode,	// 総数(利用してはいけない)
};

// RootSignatureのパラメータ番号
enum RootParameterIndex {
	kRootParamMaterial = 0,			// b0 (PS)  マテリアル
	kRootParamTransformation = 1,	// b0 (VS)  WVP行列
	kRootParamTexture = 2,			// t0 (PS)  テクスチャ
	kRootParamDirectionalLight = 3,	// b1 (PS)  平行光源
};

// RootSignature / InputLayout / ブレンドモードごとのPSO を管理するクラス
class GraphicsPipeline {
public:
	void Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler);
	// RootSignatureと、ブレンドモードに対応したPSOをコマンドリストに設定する
	void SetPipeline(ID3D12GraphicsCommandList* commandList, BlendMode blendMode) const;

private:
	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineStates(ID3D12Device* device, ShaderCompiler* shaderCompiler);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStates_[kCountOfBlendMode];
};
