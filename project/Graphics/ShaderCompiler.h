#pragma once
#include <string>
#include <wrl.h>
#include <dxcapi.h>

// HLSLをコンパイルするクラス(DXC)
class ShaderCompiler {
public:
	void Initialize();
	// filePath: HLSLファイルのパス / profile: "vs_6_0" など
	Microsoft::WRL::ComPtr<IDxcBlob> Compile(const std::wstring& filePath, const wchar_t* profile);

private:
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
};
