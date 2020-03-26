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
	int boneIdx;	// ボーン行列配列の対応
	DirectX::XMFLOAT3 startPos;	// ボーン始点
	DirectX::XMFLOAT3 endPos;	// ボーン終点
	std::vector<BoneNode*> children;	// 子供たちへのリンク
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

	// ボーン関係変数等
	std::vector<DirectX::XMMATRIX> boneMatrices;	// ボーン行列転送用
	std::map<std::string, BoneNode> boneMap;	// ボーンマップ
	ID3D12Resource* boneBuffer;	// ボーンバッファー
	ID3D12DescriptorHeap* boneHeap;	// ボーン用ヒープ
	DirectX::XMMATRIX* mappedbones;

	std::shared_ptr<PMDModel> _model;
	std::shared_ptr<FBXModel> _fbxModel;

	// インターフェース類用ポインタ変数生成
	D3D12_VERTEX_BUFFER_VIEW vbView;		// 頂点バッファ
	D3D12_INDEX_BUFFER_VIEW ibView;			// インデックスバッファ
	ID3D12DescriptorHeap* _rgstDescHeap;		// テクスチャとか定数とかのためのヒープ
	D3D12_SHADER_RESOURCE_VIEW_DESC _texView;	// テクスチャービュー
	UINT64 _fanceValue = 0;
	D3D12_VIEWPORT _viewPort;
	D3D12_RECT _rect;
	IDXGIAdapter* adapter = nullptr;

	ComPtr<ID3D12Fence> _fence;
	ComPtr<ID3D12Device> _dev;
	ComPtr<ID3D12DescriptorHeap> _rtvDescHeap;		// レンダーターゲットビュー用
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

	// 頂点情報を定義し、頂点バッファを作る
	// バッファ系
	ID3D12Resource* _vertexBuffer;
	ID3D12Resource* _indexBuffer;
	ID3D12Resource* _texBuffer;
	ID3D12Resource* depthBuffer;
	ID3D12DescriptorHeap* _dsvHeap;

	DirectX::TexMetadata metadata = {};
	
	std::vector<ID3D12Resource*> _backBuffer;
	std::vector<ID3D12Resource*> _peraBBuffer;
	std::vector<ID3D12Resource*> _indexBuffers;

	// 白テクスチャ用バッファ
	ID3D12Resource* WhiteTexBuff;
	ID3D12Resource* BlackTexBuff;
	std::vector<ID3D12Resource*> ModelTexBuff;
	std::vector<ID3D12Resource*> ModelToonBuff;
	ID3D12DescriptorHeap* WhiteBuffHeap;
	ID3D12DescriptorHeap* BlackBuffHeap;

	ID3D12Resource* GradestionalBuff;
	ID3D12Resource* GradestionalBuffer;
	ID3D12DescriptorHeap* GradDescHeap;

	// toon用Map
	ID3D12Resource* toonBuffer;
	std::vector<ID3D12Resource*> palette;

	// RGBAテクスチャ用変数
	ID3D12Resource* RGBAtexBuffer;
	ID3D12DescriptorHeap* rgbaRDescHeap;	// レンダーターゲットビュー用変数
	ID3D12DescriptorHeap* rgbaSDescHeap;	// シェーダリソースビュー用変数

	ID3D12Resource* peraVB;
	D3D12_VERTEX_BUFFER_VIEW peraVBV;
	ID3DBlob* _peravsBlob;
	ID3DBlob* _perapsBlob;

	// コンスタントバッファ用
	ID3D12Resource* _cBuffer;

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	DirectX::ScratchImage img;
	DirectX::XMMATRIX camera;
	DirectX::XMMATRIX projection;
	TransFromMatrices* _mappedMatrix;

	// 深度バッファビュー用
	ID3D12DescriptorHeap* depthView;

	//primitive用変数
	std::array<PrimVertex, 4> PrimVertices;
	ID3D12Resource* PrimVertBuffer;
	ID3D12Resource* PrimCBuff;
	D3D12_VERTEX_BUFFER_VIEW PrimVertBuffView;
	ID3DBlob* _primvsBlob = nullptr;
	ID3DBlob* _primpsBlob = nullptr;
	PrimTransFromMatrices primtm;
	PrimTransFromMatrices* _PrimMappedMatrix;
	ID3D12DescriptorHeap* PrimDescHeap;

	// シャドウマップ用変数
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

	// コンスタントバッファの初期化等
	// 注意点:テクスチャの初期化の後に呼び出す
	// InitTextureの後に呼び出す
	void InitConstants();
	void InitVertices();		// 頂点情報初期化
	void InitShader();			// シェーダー初期化
	void InitAdapter();			// アダプター初期化
	void InitFeatureLev();		// フューチャーレベル設定
	void InitTexture();			// テクスチャ情報初期化
	void InitCommand();			// コマンド系初期化
	void InitSwapChain(HWND hwnd);// スワップチェイン初期化
	void InitFense();			// フェンス初期化
	void InitDescHeapForRtv();	// RTVデスクリプターヒープ初期化
	void InitRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& par);	// ルートシグネチャの初期化
	void InitPeraRS(const std::vector<D3D12_ROOT_PARAMETER>& par);
	void InitPiplineState();	// パイプラインステート初期化
	void InitPeraPiplineState();
	void InitDepthBuffer();		// 深度デプスバッファ初期化
	void CreateSRV(ID3D12Resource* res, D3D12_SHADER_RESOURCE_VIEW_DESC desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void CreateRTV(CD3DX12_CPU_DESCRIPTOR_HANDLE desch, UINT heapsize, std::vector<ID3D12Resource*>& buff);
	HRESULT CreateDescHeap(D3D12_DESCRIPTOR_HEAP_DESC heapdesc, ID3D12DescriptorHeap* & heap);
	void CreateModelTextures();// モデル用テクスチャ作成用関数
	void CreateWhiteTexture();// ホワイトテクスチャ作成用関数
	void CreateBluckTexture();// ブラックテクスチャ作成用関数
	void CreateGradTexture();// グラデーションテクスチャ作成用関数
	void CreateRGBATexture();
	void InitBone();
	void CreateDsvView();
	void InitPrimRS(const std::vector<D3D12_ROOT_PARAMETER>& par);
	void InitPrimPS();
	void CreateManagerBuff(const std::string& str);
	void CloseBarrier(const std::string& str);
	void CreateGradMaterial();

	// シャドウマップ用関数
	void InitShadowRS(const std::vector<D3D12_ROOT_PARAMETER>& par);
	void InitShadowPSO();
	void CreateShadowMap();
	size_t RoundupPowerOf2(size_t size);

	// InitMaterial後に変更
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

