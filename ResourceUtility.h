#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <vector>
#include "externals/DirectXTex/DirectXTex.h"
#include "Math.h"	// VertexData

// ---- バッファ ----
// Uploadヒープ上のバッファリソースを作る
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

// 頂点バッファ(リソース + ビュー + 頂点数)
struct VertexBuffer {
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_VERTEX_BUFFER_VIEW view{};
	UINT vertexCount = 0;
};
VertexBuffer CreateVertexBuffer(ID3D12Device* device, const std::vector<VertexData>& vertices);

// 定数バッファ(リソース + Mapしたポインタ)
template <typename T>
struct ConstantBuffer {
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	T* data = nullptr;	// 書き込み用ポインタ
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const { return resource->GetGPUVirtualAddress(); }
};

template <typename T>
ConstantBuffer<T> CreateConstantBuffer(ID3D12Device* device) {
	ConstantBuffer<T> cb;
	cb.resource = CreateBufferResource(device, sizeof(T));
	cb.resource->Map(0, nullptr, reinterpret_cast<void**>(&cb.data));
	return cb;
}

// ---- テクスチャ ----
// 画像を読み込んでミップマップを作る
DirectX::ScratchImage LoadTexture(const std::string& filePath);
// metadataを基にテクスチャリソースを作る
Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);
// テクスチャにデータを転送する。戻り値の中間リソースは、転送完了まで解放しないこと
[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
	ID3D12Resource* texture,
	const DirectX::ScratchImage& mipImages,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList);
