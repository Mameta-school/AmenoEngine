#pragma once
#include <Windows.h>
#include <xaudio2.h>
#include <wrl.h>

// 音声データ
struct SoundData {
	// 波形フォーマット
	WAVEFORMATEX wfex;
	// バッファの先頭アドレス
	BYTE* pBuffer;
	// バッファサイズ
	unsigned int bufferSize;
};

// 音声再生を担当するクラス
class Audio {
public:
	// 初期化(XAudio2エンジンとマスターボイスの生成)
	void Initialize();
	// 終了処理
	void Finalize();
	// 音声データの読み込み(.wav)
	SoundData LoadWave(const char* filename);
	// 音声データ解放
	void Unload(SoundData* soundData);
	// 音声再生
	void PlayWave(const SoundData& soundData);

private:
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;
};
