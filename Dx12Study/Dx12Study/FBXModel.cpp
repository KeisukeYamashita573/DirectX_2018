#include "FBXModel.h"
#include <iostream>
#include "d3dx12.h"

#pragma comment(lib,"d3d12.lib")

FBXModel::FBXModel(const char* filepath) {
	//auto fbxManager = FbxManager::Create();
	//auto fbxScene = FbxScene::Create(fbxManager, "fbxscene");
	//FbxString FileName("Alicia_solid_MMD.FBX");
	//FbxImporter *fbxImporter = FbxImporter::Create(fbxManager, "imp");
	//fbxImporter->Initialize(FileName.Buffer(), -1, fbxManager->GetIOSettings());
	//fbxImporter->Import(fbxScene);
	//fbxImporter->Destroy();

	////メッシュデータの取得
	//FbxMesh* mesh = nullptr;
	//auto a = fbxScene->GetRootNode()->GetChild(1)->GetNodeAttribute()->GetAttributeType();
	//for (auto i = 0; i < fbxScene->GetRootNode()->GetChildCount(); i++) {
	//	if (fbxScene->GetRootNode()->GetChild(i)->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
	//		mesh = fbxScene->GetRootNode()->GetChild(i)->GetMesh();
	//		break;
	//	}
	//}

	//// 頂点情報の取得
	//vertices.resize(mesh->GetControlPointsCount());
	//for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
	//	vertices[i].pos.x = static_cast<float>(mesh->GetControlPointAt(i)[0]);
	//	vertices[i].pos.y = static_cast<float>(mesh->GetControlPointAt(i)[1]);
	//	vertices[i].pos.z = static_cast<float>(mesh->GetControlPointAt(i)[2]);
	//}

	//// インデックス情報の取得
	//idx.ByteWidth = sizeof(int) * mesh->GetPolygonVertexCount();
	//idx.PolygonVertCnt = mesh->GetPolygonVertices();
}

FBXModel::~FBXModel() {
}

std::vector<FbxVertex> FBXModel::Vertices_data() {
	return vertices;
}

FbxIdx FBXModel::Index_data() {
	return idx;
}
