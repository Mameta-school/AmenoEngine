#pragma once
#include <string>

namespace Logger {
	// ログファイル(logs/日時.log)を作る。最初に1回呼ぶ
	void Initialize();
	// ログファイルと出力ウィンドウに文字を出す
	void Log(const std::string& message);
}
