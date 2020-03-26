#pragma once
#include <DirectXMath.h>
//#include <fbxsdk.h>
#include <vector>

struct FbxVertex {
	DirectX::XMFLOAT3 pos;	// ���_���W
};

struct FbxIdx {
	int ByteWidth;	// �f�[�^��
	int* PolygonVertCnt;	// �C���f�b�N�X�f�[�^��
};

class FBXModel {
	std::vector<FbxVertex> vertices;	// ���_���
	FbxIdx idx;		// �C���f�b�N�X���
public:
	FBXModel(const char* filepath);
	~FBXModel();

	std::vector<FbxVertex> Vertices_data();
	FbxIdx Index_data();
};

