#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include "WinApp.h"

// DirectX12の土台(デバイス・コマンド・スワップチェーン・描画の前後処理)を担当するクラス
class DirectXCommon {
public:
	static constexpr UINT kBackBufferCount = 2;
	static constexpr DXGI_FORMAT kRtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	static constexpr DXGI_FORMAT kDsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	void Initialize(WinApp* winApp);
	void Finalize();

	// 描画前の処理(バリア、画面クリア、ビューポート設定など)
	void PreDraw();
	// 描画後の処理(バリア、コマンド実行、Present、GPU待ち)
	void PostDraw();
	// 初期化時のテクスチャ転送などを、今すぐ実行して完了を待つ
	void ExecuteCommandsAndWait();

	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvHeap_.Get(); }

	// SRVヒープのindex番目のハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCPUHandle(uint32_t index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGPUHandle(uint32_t index) const;

private:
	void InitDebugLayer();
	void InitDevice();
	void InitCommand();
	void InitSwapChain();
	void InitDescriptorHeaps();
	void InitRenderTargetView();
	void InitDepthStencilView();
	void InitFence();
	void InitViewport();

	// GPUの処理完了を待つ
	void WaitForGPU();
	// 次のコマンドを積めるようにallocatorとcommandListをResetする
	void ResetCommandList();

	WinApp* winApp_ = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[kBackBufferCount];

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	uint32_t descriptorSizeSRV_ = 0;
	uint32_t descriptorSizeRTV_ = 0;
	uint32_t descriptorSizeDSV_ = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kBackBufferCount]{};

	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

	UINT backBufferIndex_ = 0;
};
