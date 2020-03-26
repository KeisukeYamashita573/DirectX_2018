#pragma once
#include <vector>
#include <map>
#include <array>
#include <DirectXMath.h>

struct PmdVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	unsigned short boneIdx[2];
	unsigned char weight;
	unsigned char edge;
};

struct PmdMaterial {
	DirectX::XMFLOAT4 diffuse_color;
	float specularity;
	float specular_color[3];
	float mirror_color[3];
	unsigned char toon_index;
	unsigned char edge_flag;
	unsigned int face_vert_count;
	char texture_file_name[20];
};

struct PMDBone {
	char boneName[20];	// ボーン名
	unsigned short parentBoneIndex;	// 親ボーン番号
	unsigned short tailBoneIndex;	// 位置のボーン番号
	unsigned char boneType;	// ボーンの種類
	unsigned short ikParentBoneIndex;	// IKボーン番号
	DirectX::XMFLOAT3 bonePos;	// ボーンのヘッドの位置
};

struct VMDMaterial {
	char boneName[15];	// ボーン名
	unsigned int frameNo;	// フレーム番号
	DirectX::XMFLOAT3 Location;	// 位置
	DirectX::XMFLOAT4 Rotation; // 回転
	unsigned char Interpolation[64];	// 補完
};

struct KeyFrame {
	KeyFrame() {};
	//KeyFrame(unsigned int no, DirectX::XMFLOAT4 qua) : frameNo(no), quaternion(qua) {};
	KeyFrame(unsigned int no, DirectX::XMFLOAT4 qua, const unsigned char ax, const unsigned char ay, const unsigned char bx, const unsigned char by) : frameNo(no), quaternion(qua) {
		bz1 = DirectX::XMFLOAT2(static_cast<float>(ax) / 127.f, static_cast<float>(ay) / 127.f);
		bz2 = DirectX::XMFLOAT2(static_cast<float>(bx) / 127.f, static_cast<float>(by) / 127.f);
	};
	unsigned int frameNo;
	DirectX::XMFLOAT4 quaternion;
	DirectX::XMFLOAT2 bz1;
	DirectX::XMFLOAT2 bz2;
};

class PMDModel
{
private:
	std::vector<unsigned char> vertiesData;
	std::vector<unsigned short> indecesData;
	std::vector<PmdMaterial> materialsData;
	std::vector<std::string> texturepaths;
	std::vector<PMDBone> bonesData;
	std::vector<VMDMaterial> VmdData;
	std::map<std::string, std::vector<KeyFrame>> animation;
	std::array<char[100], 10>toonTexNames;
	unsigned int VertexNum;
	unsigned int IndexNum;
	unsigned int MaterialNum;
	unsigned int keyframeNum;
public:
	PMDModel(const char* filepath, const char* motion);
	~PMDModel();

	std::vector<unsigned char>& verties_Data();
	std::vector<unsigned short>& indeces_Data();
	const std::vector<PmdMaterial>& materials_Data() const;
	const std::vector<std::string>& texture_Paths() const;
	const std::vector<PMDBone>& bones_Data() const;
	std::map<std::string, std::vector<KeyFrame>> animation_Data() const;
	const std::array<char[100], 10> ToonData() const;
	unsigned int VertexNumSize();
	void MotionLoder(const char* motionpath);
	float Duration();
};

