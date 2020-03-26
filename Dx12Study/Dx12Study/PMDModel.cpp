#include "PMDModel.h"
#include <iostream>
#include <vector>
#include "d3dx12.h"

#pragma comment(lib,"d3d12.lib")

namespace {
	// モデルパスとテクスチャ相対パスから、アプリからテクスチャのパスを生成して返す
	// モデルの相対パス (絶対パスでも可)
	// テクスチャの相対パス(相対パスオンリー)
	// 合成された、アプリからテクスチャへのパス
	std::string GetFilePathTrans(const char* modelpath, const char* texpath) {
		std::string strModelpath = modelpath;
		std::string strTexpath = texpath;

		auto idx1 = strModelpath.rfind('/');
		auto idx2 = strModelpath.rfind("\\")==std::string::npos ? 0 : strModelpath.rfind("\\");
		auto idx = max(idx1, idx2);
		strModelpath = strModelpath.substr(0, idx1);
		auto resultString = strModelpath + '/' + strTexpath;

		return resultString;
		// rfind,substrを使用する
	}

	const float firstSize = 46.f;
	const float secondSize = 24.f;

	const float bonefirstsize = 24.f;
	
	const float motionfirstsize = 50.f;
}

PMDModel::PMDModel(const char* filepath, const char* motion)
{
	struct PMDHeader {
		char magic[3];	// ここが4バイト使われる
		// 1バイトのパディングが置かれる
		float version;
		char model_name[20];
		char comment[256];
	};

	// Header Data Road
	auto fp = fopen(filepath, "rb");
	PMDHeader header = {};
	fread(&header.magic, sizeof(header.magic), 1, fp);
	fread(&header.version, sizeof(header)-sizeof(header.magic)-1, 1, fp);

	// Vertex Data Road
	VertexNum = 0;
	fread(&VertexNum, sizeof(VertexNum), 1, fp);

	auto VertSize = sizeof(PmdVertex);
	const unsigned int vert_stride = 38;
	vertiesData.resize(VertexNum*vert_stride);
	fread(vertiesData.data(), vertiesData.size(), 1, fp);

	// Index Data Road
	IndexNum = 0;
	fread(&IndexNum, sizeof(IndexNum), 1, fp);

	indecesData.resize(IndexNum);
	fread(indecesData.data(),sizeof(unsigned short),indecesData.size(), fp);

	MaterialNum = 0;
	fread(&MaterialNum, sizeof(MaterialNum), 1, fp);

	int idx = 0;
	texturepaths.resize(MaterialNum);
	materialsData.resize(MaterialNum);
	for (auto& material : materialsData) {
		fread(&material, firstSize, 1, fp);
		fread(&material.face_vert_count, secondSize, 1, fp);
		if (strlen(material.texture_file_name) > 0) {
			texturepaths[idx] = GetFilePathTrans(filepath, material.texture_file_name);
		}
		++idx;
	}

	// ボーン情報読み込み
	unsigned short boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);

	bonesData.resize(boneNum);
	for (auto& bone : bonesData) {
		fread(&bone, bonefirstsize, 1, fp);
		fread(&bone.boneType, sizeof(unsigned char), 1, fp);
		fread(&bone.ikParentBoneIndex,sizeof(unsigned short) + sizeof(DirectX::XMFLOAT3), 1, fp);
	}

	unsigned short ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);
	for (int i = 0; i < ikNum; i++) {
		fseek(fp, 4, SEEK_CUR);
		unsigned char ikchainNum;
		fread(&ikchainNum, sizeof(ikchainNum), 1, fp);
		fseek(fp, 6, SEEK_CUR);
		fseek(fp, ikchainNum * sizeof(unsigned short), SEEK_CUR);
	}

	unsigned short skinNum = 0;
	fread(&skinNum, sizeof(skinNum), 1, fp);
	for (int i = 0; i < skinNum; i++) {
		fseek(fp, 20, SEEK_CUR);
		unsigned int vertNum = 0;
		fread(&vertNum, sizeof(vertNum), 1, fp);
		fseek(fp, 1, SEEK_CUR);
		fseek(fp, 16 * vertNum, SEEK_CUR);
	}

	unsigned char skinDispNum = 0;
	fread(&skinDispNum, sizeof(skinDispNum), 1, fp);
	fseek(fp, skinDispNum * sizeof(unsigned short), SEEK_CUR);

	unsigned char boneDispNum = 0;
	fread(&boneDispNum, sizeof(boneDispNum), 1, fp);
	fseek(fp, 50 * boneDispNum, SEEK_CUR);

	unsigned int dispBoneNum = 0;
	fread(&dispBoneNum, sizeof(dispBoneNum), 1, fp);
	fseek(fp, 3 * dispBoneNum, SEEK_CUR);

	unsigned char englishFlg = 0;
	fread(&englishFlg, sizeof(englishFlg), 1, fp);
	if (englishFlg) {
		fseek(fp, 20 + 256, SEEK_CUR);
		fseek(fp, boneNum * 20, SEEK_CUR);
		fseek(fp, (skinNum - 1) * 20, SEEK_CUR);
		fseek(fp, boneDispNum * 50, SEEK_CUR);
	}

	fread(toonTexNames.data(), sizeof(char) * 100, toonTexNames.size(), fp);

	fclose(fp);

	// モーション読み込み
	MotionLoder(motion);
}


PMDModel::~PMDModel()
{
}

std::vector<unsigned char>& PMDModel::verties_Data()
{
	return vertiesData;
}

std::vector<unsigned short>& PMDModel::indeces_Data()
{
	return indecesData;
}

const std::vector<PmdMaterial>& PMDModel::materials_Data() const
{
	return materialsData;
}

const std::vector<std::string>& PMDModel::texture_Paths() const
{
	return texturepaths;
}

const std::vector<PMDBone>& PMDModel::bones_Data() const {
	return bonesData;
}

std::map<std::string, std::vector<KeyFrame>> PMDModel::animation_Data() const {
	return animation;
}

const std::array<char[100], 10> PMDModel::ToonData() const
{
	return toonTexNames;
}

unsigned int PMDModel::VertexNumSize()
{
	return VertexNum;
}

void PMDModel::MotionLoder(const char * motionpath) {
	auto mp = fopen(motionpath, "rb");

	fseek(mp, motionfirstsize, SEEK_SET);
	keyframeNum = 0;
	fread(&keyframeNum, sizeof(keyframeNum), 1, mp);

	VmdData.resize(keyframeNum);
	for (auto& keyframe : VmdData) {
		fread(&keyframe, sizeof(keyframe.boneName), 1, mp);
		fread(&keyframe.frameNo, sizeof(keyframe.frameNo) +
			sizeof(keyframe.Location) +
			sizeof(keyframe.Rotation) +
			sizeof(keyframe.Interpolation), 1, mp);
	}

	for (auto& f : VmdData) {
		// 48 52 56 60
		animation[f.boneName].emplace_back(KeyFrame(f.frameNo, f.Rotation, f.Interpolation[48], f.Interpolation[52], f.Interpolation[56], f.Interpolation[60]));
	}
}

float PMDModel::Duration() {
	auto maxframe = 0.f;
	for (auto& a : animation) {
		for (int i = 0; i < a.second.size(); i++) {
			maxframe = max(maxframe, a.second[i].frameNo);
		}
	}
	return maxframe;
}
