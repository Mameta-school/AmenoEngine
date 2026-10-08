#include "ModelLoader.h"
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	// 中で必要となる変数の宣言
	MaterialData materialData;
	std::string line;
	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());
	// 実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連続してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	// 中で必要となる変数の宣言
	ModelData modelData;			// 構築するModelData
	std::vector<Vector4> positions;	// 位置
	std::vector<Vector3> normals;	// 法線
	std::vector<Vector2> texcoords;	// テクスチャ座標
	std::string line;				// ファイルから読んだ1行を格納するもの
	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());	// とりあえず開けなかったら止める
	// 実際にファイルを読み、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;	// 先頭の識別子を読む

		// identifierに応じた処理
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			// 面を三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');	// 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				position.x *= -1.0f;
				normal.x *= -1.0f;
				texcoord.y = 1.0f - texcoord.y;
				triangle[faceVertex] = { position, texcoord, normal };
			}
			// 頂点を逆順で登録することで、回り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
}

ModelData CreateSphereModel() {
	ModelData modelData;
	const uint32_t kSubdivision = 16;
	uint32_t vertexCount = kSubdivision * kSubdivision * 6;
	modelData.vertices.resize(vertexCount);

	const float kLonEvery = 2.0f * kPi / float(kSubdivision);
	const float kLatEvery = kPi / float(kSubdivision);

	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -kPi / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery;
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

			float nextLat = lat + kLatEvery;
			float nextLon = lon + kLonEvery;

			Vector4 posA = { std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon), 1.0f };
			Vector4 posB = { std::cos(nextLat) * std::cos(lon), std::sin(nextLat), std::cos(nextLat) * std::sin(lon), 1.0f };
			Vector4 posC = { std::cos(lat) * std::cos(nextLon), std::sin(lat), std::cos(lat) * std::sin(nextLon), 1.0f };
			Vector4 posD = { std::cos(nextLat) * std::cos(nextLon), std::sin(nextLat), std::cos(nextLat) * std::sin(nextLon), 1.0f };

			float u0 = float(lonIndex) / float(kSubdivision);
			float u1 = float(lonIndex + 1) / float(kSubdivision);
			float v0 = 1.0f - float(latIndex) / float(kSubdivision);
			float v1 = 1.0f - float(latIndex + 1) / float(kSubdivision);

			Vector2 uvA = { u0, v0 };
			Vector2 uvB = { u0, v1 };
			Vector2 uvC = { u1, v0 };
			Vector2 uvD = { u1, v1 };

			Vector3 normA = { posA.x, posA.y, posA.z };
			Vector3 normB = { posB.x, posB.y, posB.z };
			Vector3 normC = { posC.x, posC.y, posC.z };
			Vector3 normD = { posD.x, posD.y, posD.z };

			modelData.vertices[start + 0] = { posA, uvA, normA };
			modelData.vertices[start + 1] = { posB, uvB, normB };
			modelData.vertices[start + 2] = { posC, uvC, normC };

			modelData.vertices[start + 3] = { posC, uvC, normC };
			modelData.vertices[start + 4] = { posB, uvB, normB };
			modelData.vertices[start + 5] = { posD, uvD, normD };
		}
	}
	return modelData;
}
