#pragma once
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <vector>
#include <wrl.h>
#include <memory>
#include <map>
#include <string>
#include <array>
#include <functional>
#include "PMDModel.h"
#include "FBXModel.h"

using Microsoft::WRL::ComPtr;
class PMDModel;

using namespace DirectX;

struct TransFromMatrices {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX camera;
	DirectX::XMMATRIX wvp;
	DirectX::XMMATRIX lvp;
	DirectX::XMFLOAT4 eye;
};

struct PrimTransFromMatrices {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX viewproj;
	DirectX::XMMATRIX lvp;
};

struct Material {
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 Specular;
	DirectX::XMFLOAT4 ambient;
};

struct BoneNode {
	int boneIdx;	// �{�[���s��z��̑Ή�
	DirectX::XMFLOAT3 startPos;	// �{�[���n�_
	DirectX::XMFLOAT3 endPos;	// �{�[���I�_
	std::vector<BoneNode*> children;	// �q�������ւ̃����N
};

struct PrimVertex {
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;

	PrimVertex() {
		pos = XMFLOAT3(0, 0, 0);
		normal = XMFLOAT3(0, 0, 0);
		uv = XMFLOAT2(0, 0);

	}
	PrimVertex(XMFLOAT3& p, XMFLOAT3& norm, XMFLOAT2& coord) {
		pos = p;
		normal = norm;
		uv = coord;
	}
	PrimVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
		pos.x = x;
		pos.y = y;
		pos.z = z;
		normal.x = nx;
		normal.y = ny;
		normal.z = nz;
		uv.x = u;
		uv.y = v;
	}
};

struct ShadowVertex {
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;

	ShadowVertex () {
		pos = XMFLOAT3 (0, 0, 0);
		normal = XMFLOAT3 (0, 0, 0);
		uv = XMFLOAT2 (0, 0);

	}
	ShadowVertex (XMFLOAT3& p, XMFLOAT3& norm, XMFLOAT2& coord) {
		pos = p;
		normal = norm;
		uv = coord;
	}
	ShadowVertex (float x, float y, float z, float nx, float ny, float nz, float u, float v) {
		pos.x = x;
		pos.y = y;
		pos.z = z;
		normal.x = nx;
		normal.y = ny;
		normal.z = nz;
		uv.x = u;
		uv.y = v;
	}
};

struct Color {
	Color() : r(0), g(0), b(0), a(0) {}
	Color(unsigned char inr, unsigned char ing, unsigned char inb, unsigned char ina)
		:r(inr), g(ing), b(inb), a(ina) {}
	unsigned char r, g, b, a;
};

class Dx12Initer{
private:
	ID3D12Resource* ManagerMaterialBuffer;
	ID3D12Resource* ManagerTextureBuffer;
	//ID3D12Resource* ManagerGradBuffer;
	ID3D12Resource* ManagerRGBABuffer;

	TransFromMatrices tm;
	Material* _mappedMaterial;

	std::vector<Material> _materials;
	std::vector<ID3D12Resource*> _materialBuffers;
	ID3D12DescriptorHeap* _materialHeaps;

	// �{�[���֌W�ϐ���
	std::vector<DirectX::XMMATRIX> boneMatrices;	// �{�[���s��]���p
	std::map<std::string, BoneNode> boneMap;	// �{�[���}�b�v
	ID3D12Resource* boneBuffer;	// �{�[���o�b�t�@�[
	ID3D12DescriptorHeap* boneHeap;	// �{�[���p�q�[�v
	DirectX::XMMATRIX* mappedbones;

	std::shared_ptr<PMDModel> _model;
	std::shared_ptr<FBXModel> _fbxModel;

	// �C���^�[�t�F�[�X�ޗp�|�C���^�ϐ�����
	D3D12_VERTEX_BUFFER_VIEW vbView;		// ���_�o�b�t�@
	D3D12_INDEX_BUFFER_VIEW ibView;			// �C���f�b�N�X�o�b�t�@
	ID3D12DescriptorHeap* _rgstDescHeap;		// �e�N�X�`���Ƃ��萔�Ƃ��̂��߂̃q�[�v
	D3D12_SHADER_RESOURCE_VIEW_DESC _texView;	// �e�N�X�`���[�r���[
	UINT64 _fanceValue = 0;
	D3D12_VIEWPORT _viewPort;
	D3D12_RECT _rect;
	IDXGIAdapter* adapter = nullptr;

	ComPtr<ID3D12Fence> _fence;
	ComPtr<ID3D12Device> _dev;
	ComPtr<ID3D12DescriptorHeap> _rtvDescHeap;		// �����_�[�^�[�Q�b�g�r���[�p
	ComPtr<ID3D12RootSignature> _rootSignature;
	ComPtr<ID3D12RootSignature> _peraRootSignature;
	ComPtr<ID3D12CommandQueue> _commandQueue;
	ComPtr<ID3D12CommandAllocator> _commandAlloc;
	ComPtr<ID3D12GraphicsCommandList1> _commandList;
	ComPtr<IDXGIFactory5> _dxgiFactory;
	ComPtr<IDXGISwapChain4> _swapChain;
	ComPtr<ID3D12PipelineState> _pipeline;
	ComPtr<ID3D12PipelineState> _peraPipeline;

	ComPtr<ID3D12RootSignature> _primRootsignature;
	ComPtr<ID3D12PipelineState> _primPipeline;

	// ���_�����`���A���_�o�b�t�@�����
	// �o�b�t�@�n
	ID3D12Resource* _vertexBuffer;
	ID3D12Resource* _indexBuffer;
	ID3D12Resource* _texBuffer;
	ID3D12Resource* depthBuffer;
	ID3D12DescriptorHeap* _dsvHeap;

	DirectX::TexMetadata metadata = {};
	
	std::vector<ID3D12Resource*> _backBuffer;
	std::vector<ID3D12Resource*> _peraBBuffer;
	std::vector<ID3D12Resource*> _indexBuffers;

	// ���e�N�X�`���p�o�b�t�@
	ID3D12Resource* WhiteTexBuff;
	ID3D12Resource* BlackTexBuff;
	std::vector<ID3D12Resource*> ModelTexBuff;
	std::vector<ID3D12Resource*> ModelToonBuff;
	ID3D12DescriptorHeap* WhiteBuffHeap;
	ID3D12DescriptorHeap* BlackBuffHeap;

	ID3D12Resource* GradestionalBuff;
	ID3D12Resource* GradestionalBuffer;
	ID3D12DescriptorHeap* GradDescHeap;

	// toon�pMap
	ID3D12Resource* toonBuffer;
	std::vector<ID3D12Resource*> palette;

	// RGBA�e�N�X�`���p�ϐ�
	ID3D12Resource* RGBAtexBuffer;
	ID3D12DescriptorHeap* rgbaRDescHeap;	// �����_�[�^�[�Q�b�g�r���[�p�ϐ�
	ID3D12DescriptorHeap* rgbaSDescHeap;	// �V�F�[�_���\�[�X�r���[�p�ϐ�

	ID3D12Resource* peraVB;
	D3D12_VERTEX_BUFFER_VIEW peraVBV;
	ID3DBlob* _peravsBlob;
	ID3DBlob* _perapsBlob;

	// �R���X�^���g�o�b�t�@�p
	ID3D12Resource* _cBuffer;

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	DirectX::ScratchImage img;
	DirectX::XMMATRIX camera;
	DirectX::XMMATRIX projection;
	TransFromMatrices* _mappedMatrix;

	// �[�x�o�b�t�@�r���[�p
	ID3D12DescriptorHeap* depthView;

	//primitive�p�ϐ�
	std::array<PrimVertex, 4> PrimVertices;
	ID3D12Resource* PrimVertBuffer;
	ID3D12Resource* PrimCBuff;
	D3D12_VERTEX_BUFFER_VIEW PrimVertBuffView;
	ID3DBlob* _primvsBlob = nullptr;
	ID3DBlob* _primpsBlob = nullptr;
	PrimTransFromMatrices primtm;
	PrimTransFromMatrices* _PrimMappedMatrix;
	ID3D12DescriptorHeap* PrimDescHeap;

	// �V���h�E�}�b�v�p�ϐ�
	ID3D12DescriptorHeap* ShadowDSV;
	ID3D12DescriptorHeap* ShadowSRV;
	ID3D12Resource* ShadowBuffer;
	ComPtr<ID3D12RootSignature> ShadowRootsignature;
	ID3DBlob* ShadowPsBlob = nullptr;
	ID3DBlob* ShadowVsBlob = nullptr;
	ComPtr<ID3D12PipelineState> ShadowPipelineState;
	ID3D12DescriptorHeap* ShadowDescHeap;
	D3D12_VIEWPORT ShadowViewPort;
	D3D12_RECT ShadowRect;

	// �R���X�^���g�o�b�t�@�̏�������
	// ���ӓ_:�e�N�X�`���̏������̌�ɌĂяo��
	// InitTexture�̌�ɌĂяo��
	void InitConstants();
	void InitVertices();		// ���_��񏉊���
	void InitShader();			// �V�F�[�_�[������
	void InitAdapter();			// �A�_�v�^�[������
	void InitFeatureLev();		// �t���[�`���[���x���ݒ�
	void InitTexture();			// �e�N�X�`����񏉊���
	void InitCommand();			// �R�}���h�n������
	void InitSwapChain(HWND hwnd);// �X���b�v�`�F�C��������
	void InitFense();			// �t�F���X������
	void InitDescHeapForRtv();	// RTV�f�X�N���v�^�[�q�[�v������
	void InitRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& par);	// ���[�g�V�O�l�`���̏�����
	void InitPeraRS(const std::vector<D3D12_ROOT_PARAMETER>& par);
	void InitPiplineState();	// �p�C�v���C���X�e�[�g������
	void InitPeraPiplineState();
	void InitDepthBuffer();		// �[�x�f�v�X�o�b�t�@������
	void CreateSRV(ID3D12Resource* res, D3D12_SHADER_RESOURCE_VIEW_DESC desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void CreateRTV(CD3DX12_CPU_DESCRIPTOR_HANDLE desch, UINT heapsize, std::vector<ID3D12Resource*>& buff);
	HRESULT CreateDescHeap(D3D12_DESCRIPTOR_HEAP_DESC heapdesc, ID3D12DescriptorHeap* & heap);
	void CreateModelTextures();// ���f���p�e�N�X�`���쐬�p�֐�
	void CreateWhiteTexture();// �z���C�g�e�N�X�`���쐬�p�֐�
	void CreateBluckTexture();// �u���b�N�e�N�X�`���쐬�p�֐�
	void CreateGradTexture();// �O���f�[�V�����e�N�X�`���쐬�p�֐�
	void CreateRGBATexture();
	void InitBone();
	void CreateDsvView();
	void InitPrimRS(const std::vector<D3D12_ROOT_PARAMETER>& par);
	void InitPrimPS();
	void CreateManagerBuff(const std::string& str);
	void CloseBarrier(const std::string& str);
	void CreateGradMaterial();

	// �V���h�E�}�b�v�p�֐�
	void InitShadowRS(const std::vector<D3D12_ROOT_PARAMETER>& par);
	void InitShadowPSO();
	void CreateShadowMap();
	size_t RoundupPowerOf2(size_t size);

	// InitMaterial��ɕύX
	void InitMaterialTest();
	void RecursiveMatrixMultiply(BoneNode& node,DirectX::XMMATRIX& inmat);
	void RotateBoneA(const char* bonename, const DirectX::XMFLOAT4& q);
	void RotateBoneB(const char* bonename,const DirectX::XMFLOAT4& q,const DirectX::XMFLOAT4& q2 = DirectX::XMFLOAT4(), float t = 0.f);
	void AnimationUpDate(int frameno);
	int frameno = 0;

	float GetBezierYValueFromXWithBisection(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, const unsigned int limit = 16);

public:
	Dx12Initer(HINSTANCE h, HWND hwnd);
	~Dx12Initer();

	void Update();
	void ExecuteCommand();
	void WaitExecute();
};

