#pragma once
#include <DirectXMath.h>
//#include <fbxsdk.h>
#include <vector>

struct FbxVertex {
	DirectX::XMFLOAT3 pos;	// 頂点座標
};

struct FbxIdx {
	int ByteWidth;	// データ量
	int* PolygonVertCnt;	// インデックスデータ数
};

class FBXModel {
	std::vector<FbxVertex> vertices;	// 頂点情報
	FbxIdx idx;		// インデックス情報
public:
	FBXModel(const char* filepath);
	~FBXModel();

	std::vector<FbxVertex> Vertices_data();
	FbxIdx Index_data();
};

