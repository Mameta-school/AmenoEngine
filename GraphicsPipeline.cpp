#include "GraphicsPipeline.h"
#include "DirectXCommon.h"
#include "Logger.h"
#include <cassert>

using Microsoft::WRL::ComPtr;

namespace {
	// ブレンドモードに対応したBlendStateを作る
	D3D12_BLEND_DESC CreateBlendDesc(BlendMode mode) {
		D3D12_BLEND_DESC desc{};
		D3D12_RENDER_TARGET_BLEND_DESC& rt = desc.RenderTarget[0];
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// ブレンドなし
		if (mode == kBlendModeNone) {
			rt.BlendEnable = FALSE;
			return desc;
		}

		rt.BlendEnable = TRUE;
		// αチャンネルの扱いは全モード共通
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt.DestBlendAlpha = D3D12_BLEND_ZERO;

		switch (mode) {
		case kBlendModeNormal:
			rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rt.BlendOp = D3D12_BLEND_OP_ADD;
			rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;
		case kBlendModeAdd:
			rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rt.BlendOp = D3D12_BLEND_OP_ADD;
			rt.DestBlend = D3D12_BLEND_ONE;
			break;
		case kBlendModeSubtract:
			rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			rt.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			rt.DestBlend = D3D12_BLEND_ONE;
			break;
		case kBlendModeMultiply:
			rt.SrcBlend = D3D12_BLEND_ZERO;
			rt.BlendOp = D3D12_BLEND_OP_ADD;
			rt.DestBlend = D3D12_BLEND_SRC_COLOR;
			break;
		case kBlendModeScreen:
			rt.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			rt.BlendOp = D3D12_BLEND_OP_ADD;
			rt.DestBlend = D3D12_BLEND_ONE;
			break;
		default:
			break;
		}
		return desc;
	}
}

void GraphicsPipeline::Initialize(ID3D12Device* device, ShaderCompiler* shaderCompiler) {
	CreateRootSignature(device);
	CreatePipelineStates(device, shaderCompiler);
}

void GraphicsPipeline::SetPipeline(ID3D12GraphicsCommandList* commandList, BlendMode blendMode) const {
	// RootSignatureを設定。PSOに設定しているけど別途設定が必要
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineStates_[blendMode].Get());
}

void GraphicsPipeline::CreateRootSignature(ID3D12Device* device) {
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootParameter作成
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[kRootParamMaterial].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[kRootParamMaterial].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[kRootParamMaterial].Descriptor.ShaderRegister = 0;

	rootParameters[kRootParamTransformation].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[kRootParamTransformation].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[kRootParamTransformation].Descriptor.ShaderRegister = 0;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;								// 0から始まる
	descriptorRange[0].NumDescriptors = 1;									// 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;			// SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[kRootParamTexture].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[kRootParamTexture].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[kRootParamTexture].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[kRootParamTexture].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootParameters[kRootParamDirectionalLight].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[kRootParamDirectionalLight].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[kRootParamDirectionalLight].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	// Sampler
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) {
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
	}
	// バイナリを元に生成
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void GraphicsPipeline::CreatePipelineStates(ID3D12Device* device, ShaderCompiler* shaderCompiler) {
	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;		// 裏面を表示しない
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;	// 三角形の中を塗りつぶす

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;								// Depthの機能を有効化する
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;		// 書き込みします
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;		// 近ければ描画される

	// Shaderをコンパイルする
	ComPtr<IDxcBlob> vertexShaderBlob = shaderCompiler->Compile(L"Object3d.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);
	ComPtr<IDxcBlob> pixelShaderBlob = shaderCompiler->Compile(L"Object3d.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature_.Get();
	desc.InputLayout = inputLayoutDesc;
	desc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	desc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	desc.RasterizerState = rasterizerDesc;
	// 書き込むRTVの情報
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DirectXCommon::kRtvFormat;
	// 利用する形状のタイプ
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// DepthStencilの設定
	desc.DepthStencilState = depthStencilDesc;
	desc.DSVFormat = DirectXCommon::kDsvFormat;

	// ブレンドモードごとにPSOを生成
	for (int i = 0; i < kCountOfBlendMode; ++i) {
		desc.BlendState = CreateBlendDesc(static_cast<BlendMode>(i));
		HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineStates_[i]));
		assert(SUCCEEDED(hr));
	}
}
