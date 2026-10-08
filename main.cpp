#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <string>
#include <strsafe.h>
#include <wrl.h>

#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#include "Math.h"
#include "DebugCamera.h"
#include "Logger.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "Input.h"
#include "Audio.h"
#include "ShaderCompiler.h"
#include "GraphicsPipeline.h"
#include "ImGuiManager.h"
#include "ResourceUtility.h"
#include "ModelLoader.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// ===============================================
//  リークチェック / クラッシュダンプ
// ===============================================
struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		// リソースリークチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processIdとクラッシュの発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	return EXCEPTION_EXECUTE_HANDLER;
}

// ===============================================
//  テクスチャ(読み込み + SRV作成)
// ===============================================
struct Texture {
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediate;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};
};

// 画像を読んで転送し、SRVヒープの srvIndex 番目にSRVを作る
Texture LoadTextureAndCreateSRV(DirectXCommon& dxCommon, const std::string& filePath, uint32_t srvIndex) {
	Texture texture;
	DirectX::ScratchImage mipImages = LoadTexture(filePath);
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	texture.resource = CreateTextureResource(dxCommon.GetDevice(), metadata);
	texture.intermediate = UploadTextureData(texture.resource.Get(), mipImages, dxCommon.GetDevice(), dxCommon.GetCommandList());

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	dxCommon.GetDevice()->CreateShaderResourceView(texture.resource.Get(), &srvDesc, dxCommon.GetSrvCPUHandle(srvIndex));
	texture.srvHandleGPU = dxCommon.GetSrvGPUHandle(srvIndex);
	return texture;
}

// ===============================================
//  本体
// ===============================================
static void Run() {
	// ---------- 基盤 ----------
	WinApp winApp;
	winApp.Initialize();

	DirectXCommon dxCommon;
	dxCommon.Initialize(&winApp);
	ID3D12Device* device = dxCommon.GetDevice();
	ID3D12GraphicsCommandList* commandList = dxCommon.GetCommandList();

	Input input;
	input.Initialize(winApp.GetHInstance(), winApp.GetHwnd());

	Audio audio;
	audio.Initialize();
	SoundData soundData1 = audio.LoadWave("Resources/Alarm01.wav");

	ShaderCompiler shaderCompiler;
	shaderCompiler.Initialize();

	GraphicsPipeline pipeline;
	pipeline.Initialize(device, &shaderCompiler);

	// ---------- モデル(頂点バッファ) ----------
	// 球体
	ModelData sphereModelData = CreateSphereModel();
	VertexBuffer sphereVB = CreateVertexBuffer(device, sphereModelData.vertices);
	// 平面
	ModelData planeModelData = LoadObjFile("resources", "plane.obj");
	VertexBuffer planeVB = CreateVertexBuffer(device, planeModelData.vertices);
	// フェンス
	ModelData fenceModelData = LoadObjFile("resources", "fence.obj");
	VertexBuffer fenceVB = CreateVertexBuffer(device, fenceModelData.vertices);

	// スプライト
	std::vector<VertexData> spriteVertices(4);
	spriteVertices[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	spriteVertices[0].texcoord = { 0.0f, 1.0f };
	spriteVertices[0].normal = { 0.0f, 0.0f, -1.0f };
	spriteVertices[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	spriteVertices[1].texcoord = { 0.0f, 0.0f };
	spriteVertices[1].normal = { 0.0f, 0.0f, -1.0f };
	spriteVertices[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	spriteVertices[2].texcoord = { 1.0f, 1.0f };
	spriteVertices[2].normal = { 0.0f, 0.0f, -1.0f };
	spriteVertices[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	spriteVertices[3].texcoord = { 1.0f, 0.0f };
	spriteVertices[3].normal = { 0.0f, 0.0f, -1.0f };
	VertexBuffer spriteVB = CreateVertexBuffer(device, spriteVertices);

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;

	// ---------- 定数バッファ ----------
	// マテリアル(球・平面用)
	ConstantBuffer<Material> material = CreateConstantBuffer<Material>(device);
	material.data->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	material.data->enableLighting = true;
	material.data->uvTransform = MakeIdentity4x4();

	// マテリアル(スプライト用)
	ConstantBuffer<Material> materialSprite = CreateConstantBuffer<Material>(device);
	materialSprite.data->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialSprite.data->enableLighting = false;
	materialSprite.data->uvTransform = MakeIdentity4x4();

	// 平行光源
	ConstantBuffer<DirectionalLight> directionalLight = CreateConstantBuffer<DirectionalLight>(device);
	directionalLight.data->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLight.data->direction = { 0.0f, -1.0f, 1.0f };
	directionalLight.data->intensity = 1.0f;

	// WVP行列
	ConstantBuffer<TransformationMatrix> wvpSphere = CreateConstantBuffer<TransformationMatrix>(device);
	wvpSphere.data->WVP = MakeIdentity4x4();
	wvpSphere.data->World = MakeIdentity4x4();

	ConstantBuffer<TransformationMatrix> wvpPlane = CreateConstantBuffer<TransformationMatrix>(device);
	wvpPlane.data->WVP = MakeIdentity4x4();
	wvpPlane.data->World = MakeIdentity4x4();

	ConstantBuffer<TransformationMatrix> wvpSprite = CreateConstantBuffer<TransformationMatrix>(device);
	wvpSprite.data->WVP = MakeIdentity4x4();
	wvpSprite.data->World = MakeIdentity4x4();

	// ---------- Transform / カメラ ----------
	Transform transformSphere{ {0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f} };
	Transform transformPlane{ {0.5f, 0.5f, 0.5f}, {0.0f, 3.141592f, 0.0f}, { 1.0f, 0.0f, 0.0f} };
	Transform transformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform cameraTransform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f} };
	Transform uvTransformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	const float aspectRatio = float(WinApp::kClientWidth) / float(WinApp::kClientHeight);

	// 通常カメラ
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, aspectRatio, 0.1f, 100.0f);

	// デバッグカメラ
	DebugCamera debugCamera;
	debugCamera.Initialize(aspectRatio);
	bool useDebugCamera = false;

	// スプライト用
	Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
	Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
	Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	wvpSprite.data->WVP = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
	wvpSprite.data->World = worldMatrixSprite;

	// ---------- テクスチャ ----------
	// SRVヒープの0番はImGuiが使うので、1番・2番を使う
	Texture textureUvChecker = LoadTextureAndCreateSRV(dxCommon, "resources/uvChecker.png", 1);
	Texture textureMonsterBall = LoadTextureAndCreateSRV(dxCommon, "resources/monsterBall.png", 2);
	// 転送コマンドを実行して完了を待つ
	dxCommon.ExecuteCommandsAndWait();

	bool useMonsterBall = false;
	int currentBlendMode = kBlendModeNormal;

	// ---------- ImGui ----------
	ImGuiManager imGui;
	imGui.Initialize(&winApp, &dxCommon);

	// ===============================================
	//  ゲームループ
	// ===============================================
	Logger::Log("Start game loop");

	while (true) {
		// ウィンドウの×ボタンが押されたら抜ける
		if (winApp.ProcessMessage()) {
			break;
		}

		imGui.Begin();

		// ---------- 更新処理 ----------
		input.Update();

		// 数字の0キーが押されていたら
		if (input.PushKey(DIK_0)) {
			OutputDebugStringA("HIT 0\n");
		}

		// スペースキーが押されたら音を再生
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			audio.PlayWave(soundData1);
		}

		// デバッグカメラの更新
		debugCamera.Update(input.GetKeys());
		const Matrix4x4& view = useDebugCamera ? debugCamera.GetViewMatrix() : viewMatrix;
		const Matrix4x4& proj = useDebugCamera ? debugCamera.GetProjectionMatrix() : projectionMatrix;

		// 球体の行列更新
		Matrix4x4 worldMatrixSphere = MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
		wvpSphere.data->WVP = Multiply(worldMatrixSphere, Multiply(view, proj));
		wvpSphere.data->World = worldMatrixSphere;

		// 平面の行列更新
		Matrix4x4 worldMatrixPlane = MakeAffineMatrix(transformPlane.scale, transformPlane.rotate, transformPlane.translate);
		wvpPlane.data->WVP = Multiply(worldMatrixPlane, Multiply(view, proj));
		wvpPlane.data->World = worldMatrixPlane;

		// スプライトの更新
		worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
		wvpSprite.data->WVP = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
		wvpSprite.data->World = worldMatrixSprite;

		// UVTransformの更新
		Matrix4x4 uvTransformMatrix = MakeAffineMatrix(uvTransformSprite.scale, uvTransformSprite.rotate, uvTransformSprite.translate);
		materialSprite.data->uvTransform = uvTransformMatrix;

#ifdef USE_IMGUI
		ImGui::Begin("Settings");
		if (ImGui::CollapsingHeader("Camera")) {
			ImGui::Checkbox("DebugCamera (Arrow/WASD/QE)", &useDebugCamera);
		}

		if (ImGui::CollapsingHeader("Sphere Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("Scale##Sphere", &transformSphere.scale.x, 0.01f);
			ImGui::DragFloat3("Translate##Sphere", &transformSphere.translate.x, 0.01f);
			ImGui::DragFloat3("Rotate##Sphere", &transformSphere.rotate.x, 0.01f);
		}

		if (ImGui::CollapsingHeader("Plane Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("Scale##Plane", &transformPlane.scale.x, 0.01f);
			ImGui::DragFloat3("Translate##Plane", &transformPlane.translate.x, 0.01f);
			ImGui::DragFloat3("Rotate##Plane", &transformPlane.rotate.x, 0.01f);
		}

		if (ImGui::CollapsingHeader("Material")) {
			ImGui::ColorEdit4("Color", &material.data->color.x);

			const char* lightingItems[] = { "None", "Lambert", "Half Lambert" };
			ImGui::Combo("Lighting", &material.data->enableLighting, lightingItems, IM_ARRAYSIZE(lightingItems));
		}

		if (ImGui::CollapsingHeader("Blend", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* blendItems[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Screen" };
			ImGui::Combo("BlendMode", &currentBlendMode, blendItems, IM_ARRAYSIZE(blendItems));
		}

		if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::ColorEdit3("LightColor", &directionalLight.data->color.x);
			if (ImGui::DragFloat3("LightDirection", &directionalLight.data->direction.x, 0.01f, -1.0f, 1.0f)) {
				// 入力された値を単位ベクトルに正規化する
				Vector3& dir = directionalLight.data->direction;
				float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
				if (length > 0.0001f) {
					dir.x /= length;
					dir.y /= length;
					dir.z /= length;
				}
			}
			ImGui::DragFloat("LightIntensity", &directionalLight.data->intensity, 0.01f, 0.0f, 10.0f);
		}

		if (ImGui::CollapsingHeader("Texture")) {
			int textureIndex = useMonsterBall ? 1 : 0;
			ImGui::RadioButton("resources/uvChecker.png", &textureIndex, 0);
			ImGui::RadioButton("resources/monsterBall.png", &textureIndex, 1);
			useMonsterBall = (textureIndex == 1);
		}
		ImGui::End();
#endif
		imGui.End();

		// ---------- 描画処理 ----------
		dxCommon.PreDraw();

		// RootSignature と PSOを設定
		pipeline.SetPipeline(commandList, static_cast<BlendMode>(currentBlendMode));
		// 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// マテリアル / 平行光源 / テクスチャ
		commandList->SetGraphicsRootConstantBufferView(kRootParamMaterial, material.GetGPUAddress());
		commandList->SetGraphicsRootConstantBufferView(kRootParamDirectionalLight, directionalLight.GetGPUAddress());
		commandList->SetGraphicsRootDescriptorTable(kRootParamTexture,
			useMonsterBall ? textureMonsterBall.srvHandleGPU : textureUvChecker.srvHandleGPU);

		// ------------------------------------------
		//  球体 (Sphere) の描画
		// ------------------------------------------
		//commandList->SetGraphicsRootConstantBufferView(kRootParamTransformation, wvpSphere.GetGPUAddress());
		//commandList->IASetVertexBuffers(0, 1, &sphereVB.view);
		//commandList->DrawInstanced(sphereVB.vertexCount, 1, 0, 0);

		// ------------------------------------------
		//  平面 (Plane) の描画
		// ------------------------------------------
		commandList->SetGraphicsRootConstantBufferView(kRootParamTransformation, wvpPlane.GetGPUAddress());
		commandList->IASetVertexBuffers(0, 1, &planeVB.view);
		commandList->DrawInstanced(planeVB.vertexCount, 1, 0, 0);

		// ------------------------------------------
		//  スプライト (Sprite) の描画
		// ------------------------------------------
		//commandList->SetGraphicsRootDescriptorTable(kRootParamTexture, textureUvChecker.srvHandleGPU);
		//commandList->SetGraphicsRootConstantBufferView(kRootParamMaterial, materialSprite.GetGPUAddress());
		//commandList->IASetVertexBuffers(0, 1, &spriteVB.view);
		//commandList->SetGraphicsRootConstantBufferView(kRootParamTransformation, wvpSprite.GetGPUAddress());
		//commandList->IASetIndexBuffer(&indexBufferViewSprite);
		//commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// ImGuiの描画コマンドを積む
		imGui.Draw(commandList);

		dxCommon.PostDraw();
	}

	// ---------- 終了処理 ----------
	Logger::Log("End game loop");

	imGui.Finalize();
	audio.Unload(&soundData1);
	audio.Finalize();
	dxCommon.Finalize();
	winApp.Finalize();
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	D3DResourceLeakChecker leakCheck;

	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	SetUnhandledExceptionFilter(ExportDump);
	Logger::Initialize();

	// ComPtrなどはRun()を抜けるときに解放される(CoUninitializeより前)
	Run();

	CoUninitialize();
	return 0;
}
