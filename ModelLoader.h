#pragma once
#include <string>
#include "Math.h"	// ModelData, MaterialData, VertexData など

// .mtlファイルを読む
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
// .objファイルを読む(三角形のみ対応)
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
// 球の頂点データを生成する
ModelData CreateSphereModel();
