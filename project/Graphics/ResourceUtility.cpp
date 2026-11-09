#include "ResourceUtility.h"
#include "StringUtility.h"
#include "externals/DirectXTex/d3dx12.h"
#include <cassert>
#include <cstring>

using Microsoft::WRL::ComPtr;

ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	// バッファの場合はこれらを1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際に頂点リソースを作る
	ComPtr<ID3D12Resource> vertexResource;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

	return vertexResource;
}

VertexBuffer CreateVertexBuffer(ID3D12Device* device, const std::vector<VertexData>& vertices) {
	assert(!vertices.empty());
	const size_t sizeInBytes = sizeof(VertexData) * vertices.size();

	VertexBuffer vb;
	vb.resource = CreateBufferResource(device, sizeInBytes);
	vb.vertexCount = UINT(vertices.size());

	// 頂点バッファビュー
	vb.view.BufferLocation = vb.resource->GetGPUVirtualAddress();
	vb.view.SizeInBytes = UINT(sizeInBytes);
	vb.view.StrideInBytes = sizeof(VertexData);

	// 頂点データを書き込む
	VertexData* mapped = nullptr;
	vb.resource->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	std::memcpy(mapped, vertices.data(), sizeInBytes);
	vb.resource->Unmap(0, nullptr);
	return vb;
}

DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	// テクスチャファイルを読んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータを返す
	return mipImages;
}

ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);								// Textureの幅
	resourceDesc.Height = UINT(metadata.height);							// Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);					// mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);				// 奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format;									// TextureのFormat
	resourceDesc.SampleDesc.Count = 1;										// サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);	// Textureの次元数。普段使っているのは2次元

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resourceの生成
	ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,					// Heapの設定
		D3D12_HEAP_FLAG_NONE,				// Heapの特殊な設定。特になし。
		&resourceDesc,						// Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,		// データ転送される設定
		nullptr,							// Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)				// 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

ComPtr<ID3D12Resource> UploadTextureData(
	ID3D12Resource* texture,
	const DirectX::ScratchImage& mipImages,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA> subResources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subResources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subResources.size()));
	ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subResources.size()), subResources.data());

	// Textureへの転送後は利用できるよう、COPY_DEST から GENERIC_READ へResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}
