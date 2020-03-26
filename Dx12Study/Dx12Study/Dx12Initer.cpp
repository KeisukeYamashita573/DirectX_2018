#include "Dx12Initer.h"
#include "Application.h"
#include <vector>
#include <d3dcompiler.h>
#include <tchar.h>
#include <random>
#include <stdio.h>
#include <d3d12sdklayers.h>
#include "Dx12BufferManager.h"
#include <cassert>
#include <sstream>
#include <algorithm>
#include <iostream>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")		// シェーダコンパイルに必要
#pragma comment(lib,"DirectXTex.lib")

namespace {
	std::wstring StringToWString(const std::string& str) {
		auto ssize = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.data(),
			str.length(),
			nullptr,
			0);
		std::wstring ret;
		ret.resize(ssize);
		ssize = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.data(),
			str.length(),
			&ret[0],
			ssize);
		return ret;
	}

	float Magnitude(const XMFLOAT3& var) {
		return sqrt(var.x * var.x + var.y * var.y + var.z * var.z);
	}
	XMFLOAT3 operator-(XMFLOAT3& var, XMFLOAT3& val) {
		return XMFLOAT3(var.x - val.x, var.y - val.y, var.z - val.z);
	}

	XMFLOAT3 Normalize(const XMFLOAT3& vec) {
		float Mag = Magnitude(vec);
		XMFLOAT3 vector(0, 0, 0);
		vector.x = vec.x / Mag;
		vector.y = vec.y / Mag;
		vector.z = vec.z / Mag;
		return vector;
	}
}

// 頂点情報
struct Vertex {
	XMFLOAT3 pos;	// 頂点座標
	XMFLOAT2 uv;	// UV情報
};

// 頂点情報初期化
void Dx12Initer::InitVertices(){
	Vertex vertices[] = {
		XMFLOAT3(-1,-1,0), XMFLOAT2(0,1),
		XMFLOAT3(-1,1,0), XMFLOAT2(0,0),
		XMFLOAT3(1,-1,0), XMFLOAT2(1,1),
		XMFLOAT3(1,1,0), XMFLOAT2(1,0),
	};

	PrimVertices = { {
		{XMFLOAT3(-100,0.4f,-100),XMFLOAT3(0,1,0),XMFLOAT2(0,0)},
		{XMFLOAT3(-100,0.4f,100),XMFLOAT3(0,1,0),XMFLOAT2(0,1)},
		{XMFLOAT3(100,0.4f,-100),XMFLOAT3(0,1,0),XMFLOAT2(1,0)},
		{XMFLOAT3(100,0.4f,100),XMFLOAT3(0,1,0),XMFLOAT2(1,1)}
	} };
	
	// ペラポリ用頂点情報
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&peraVB));

	peraVBV.BufferLocation = peraVB->GetGPUVirtualAddress();
	peraVBV.SizeInBytes = sizeof(vertices);
	peraVBV.StrideInBytes = sizeof(Vertex);

	D3D12_RANGE peravertexrange = { 0,0 };
	Vertex* peravbuffptr = nullptr;
	result = peraVB->Map(0, &peravertexrange, (void**)&peravbuffptr);
	std::copy(std::begin(vertices), std::end(vertices), peravbuffptr);
	peraVB->Unmap(0, &peravertexrange);

	// 床表示用頂点情報
	result = _dev->CreateCommittedResource(
	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	D3D12_HEAP_FLAG_NONE,
	&CD3DX12_RESOURCE_DESC::Buffer(sizeof(PrimVertices)),
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&PrimVertBuffer));

	PrimVertBuffView.BufferLocation = PrimVertBuffer->GetGPUVirtualAddress();
	PrimVertBuffView.SizeInBytes = sizeof(PrimVertices);
	PrimVertBuffView.StrideInBytes = sizeof(PrimVertex);

	D3D12_RANGE PrimVertRange = { 0,0 };
	PrimVertex* PrimVertBuffptr = nullptr;
	result = PrimVertBuffer->Map(0, &PrimVertRange, (void**)&PrimVertBuffptr);
	std::copy(PrimVertices.begin(), PrimVertices.end(), PrimVertBuffptr);
	PrimVertBuffer->Unmap(0, &PrimVertRange);

	// 頂点情報を入れるbufferを作る
	auto vdata = _model->verties_Data();
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),	// CPUからGPUへ転送する
		D3D12_HEAP_FLAG_NONE,								// 特別な指定なし
		&CD3DX12_RESOURCE_DESC::Buffer(vdata.size()),	// サイズ
		D3D12_RESOURCE_STATE_GENERIC_READ,					// よくわからん
		nullptr,											// nullptrでいい
		IID_PPV_ARGS(&_vertexBuffer)						// いつもの
		);

	// インデックス情報を入れるbufferを作る
	auto idata = _model->indeces_Data();
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(idata.size() * sizeof(idata[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer)
	);

	// 作ったbufferに頂点情報を入れる
	D3D12_RANGE vertexrange = { 0,0 };
	// Map関数の第３引数に突っ込むやつ
	//PmdVertex* vbuffptr = nullptr;
	unsigned char* vbuffptr = nullptr;
	result = _vertexBuffer->Map(0, &vertexrange, (void**)&vbuffptr);
	// vbufferに入れていく
	std::copy(vdata.begin(),vdata.end(), vbuffptr);
	// MapしたらUnmapするのを忘れないようにする
	_vertexBuffer->Unmap(0, &vertexrange);

	D3D12_RANGE indexrange = { 0,0 };
	unsigned short* ibuffptr = nullptr;
	// マップするよ宣言
	result = _indexBuffer->Map(0, &indexrange, (void**)&ibuffptr);
	// ibuffptrに情報を入れていく
	std::copy(idata.begin(), idata.end(), ibuffptr);
	// マップ終わった宣言
	_indexBuffer->Unmap(0, &indexrange);

	// GPUへ頂点バッファビューにして投げれるようにする
	vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();	//頂点バッファのGPUにおけるアドレスを記録
	//vbView.SizeInBytes = sizeof(vertices);							// データ全体のサイズを伝える
	vbView.SizeInBytes = vdata.size();
	vbView.StrideInBytes = sizeof(PmdVertex)-2;						// 次のデータまでの距離
	//vbView.StrideInBytes = vdata.size();

	// インデックス情報を入れていく
	ibView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();	// インデックスバッファのGPUにおけるアドレスを記録
	ibView.Format = DXGI_FORMAT_R16_UINT;							// データのフォーマット
	ibView.SizeInBytes = idata.size() * sizeof(idata[0]);		// データ全体のサイズを伝える

}

// シェーダー初期化
void Dx12Initer::InitShader(){
	ID3DBlob* error = nullptr;
	ID3DBlob* peraerror = nullptr;
	ID3DBlob* primerror = nullptr;
	// シェーダのコンパイルをする
	auto result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_vsBlob, &error);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_psBlob, &error);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vsShadow", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &ShadowVsBlob, &error);
	assert(result == S_OK);

	result = D3DCompileFromFile(L"pera.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_peravsBlob, &peraerror);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"pera.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_perapsBlob, &peraerror);
	assert(result == S_OK);

	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_primvsBlob, &primerror);
	assert(result == S_OK);
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_primpsBlob, &primerror);
	assert(result == S_OK);
	

	D3D12_DESCRIPTOR_RANGE descTblRange[5] = {};
	std::vector<D3D12_ROOT_PARAMETER> rootparam = {};
	rootparam.resize(4);

	// b[0]
	descTblRange[0].NumDescriptors = 1;
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[0].BaseShaderRegister = 0;
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// b[1]
	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].BaseShaderRegister = 1;
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// t[1]
	descTblRange[2].NumDescriptors = 3;
	descTblRange[2].BaseShaderRegister = 0;
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// b[2]
	descTblRange[3].NumDescriptors = 1;
	descTblRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[3].BaseShaderRegister = 2;
	descTblRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[4].NumDescriptors = 1;
	descTblRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTblRange[4].BaseShaderRegister = 3;
	descTblRange[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE peraDescTblRange[2] = {};
	std::vector<D3D12_ROOT_PARAMETER> peraRootParam;
	peraRootParam.resize(2);

	peraDescTblRange[0].NumDescriptors = 1;
	peraDescTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	peraDescTblRange[0].BaseShaderRegister = 0;
	peraDescTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	peraDescTblRange[1].NumDescriptors = 1;
	peraDescTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	peraDescTblRange[1].BaseShaderRegister = 1;
	peraDescTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// プリミティブ用
	D3D12_DESCRIPTOR_RANGE PrimDescTblRange[2] = {};
	std::vector<D3D12_ROOT_PARAMETER> PrimRootParam;
	PrimRootParam.resize(2);

	PrimDescTblRange[0].NumDescriptors = 1;
	PrimDescTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	PrimDescTblRange[0].BaseShaderRegister = 0;
	PrimDescTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	PrimDescTblRange[1].NumDescriptors = 1;
	PrimDescTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	PrimDescTblRange[1].BaseShaderRegister = 0;
	PrimDescTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// デスクリプターテーブル設定
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[0].DescriptorTable.pDescriptorRanges = descTblRange;
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootparam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[2].DescriptorTable.pDescriptorRanges = &descTblRange[3];
	rootparam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootparam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootparam[3].DescriptorTable.pDescriptorRanges = &descTblRange[4];
	rootparam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	peraRootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	peraRootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	peraRootParam[0].DescriptorTable.pDescriptorRanges = peraDescTblRange;
	peraRootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	peraRootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	peraRootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	peraRootParam[1].DescriptorTable.pDescriptorRanges = &peraDescTblRange[1];
	peraRootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	PrimRootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	PrimRootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	PrimRootParam[0].DescriptorTable.pDescriptorRanges = PrimDescTblRange;
	PrimRootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	PrimRootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	PrimRootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	PrimRootParam[1].DescriptorTable.pDescriptorRanges = &PrimDescTblRange[1];
	PrimRootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ルートシグネチャ初期化
	InitRootSignature(rootparam);
	InitPeraRS(peraRootParam);
	InitPrimRS(PrimRootParam);
	InitShadowRS(rootparam);
	// パイプラインステート初期化
	InitPiplineState();
	InitPeraPiplineState();
	InitPrimPS();
	InitShadowPSO();
}

// アダプター初期化
void Dx12Initer::InitAdapter(){
	// グラフィックスアダプタを列挙する
	std::vector<IDXGIAdapter*> adapters;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(adapter);
	}

	// 列挙した中からNVIDIAのやつを探す
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			adapter = adpt;
			break;
		}
	}
}

// フューチャーレベル初期化
void Dx12Initer::InitFeatureLev(){
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;
	for (auto lev : levels) {
		if (S_OK == D3D12CreateDevice(adapter, lev, IID_PPV_ARGS(&_dev))) {
			featureLevel = lev;
			break;
		}
	}
}

// コンスタンス初期化
void Dx12Initer::InitConstants(){
	Application& app = Application::Instance();
	auto wsize = app.GetSize();
	//XMMATRIX matrix = XMMatrixIdentity();// XMMatrixRotationY(XM_PI / 4.f);

	auto size = sizeof(TransFromMatrices);
	const auto aligment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	size = (size + aligment-1)&~(aligment-1);

	XMFLOAT3 eye(0, 20, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	XMVECTOR veye = XMLoadFloat3(&eye);
	XMVECTOR vtarget = XMLoadFloat3(&target);
	XMVECTOR vup = XMLoadFloat3(&up);
	XMFLOAT3 lighteye(1, -1, 1);
	XMVECTOR vlighteye = XMLoadFloat3(&lighteye);
	auto angle = 0.f;
	tm.world = XMMatrixRotationY(angle);
	tm.wvp = tm.world * XMMatrixLookAtLH(veye, vtarget, vup)*
		XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(wsize.w) / static_cast<float>(wsize.h), 0.1f, 300.f);
	auto lightpos = vlighteye * Magnitude(target - eye);
	tm.lvp = XMMatrixLookAtLH(lightpos, vtarget, vup) * XMMatrixOrthographicLH(40, 40, 0.1f, 300.f);

	D3D12_DESCRIPTOR_HEAP_DESC ConDescHeapDesc = {};
	ConDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ConDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ConDescHeapDesc.NumDescriptors = 1;
	ConDescHeapDesc.NodeMask = 0;

	auto result = CreateDescHeap(ConDescHeapDesc, _rgstDescHeap);

	Dx12BufferManager::Instance().CreateConBuffer(_dev, size, _cBuffer, _rgstDescHeap);

	D3D12_DESCRIPTOR_HEAP_DESC PrimConDescHeapDesc = {};
	PrimConDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	PrimConDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	PrimConDescHeapDesc.NumDescriptors = 1;
	PrimConDescHeapDesc.NodeMask = 0;

	result = CreateDescHeap(PrimConDescHeapDesc, PrimDescHeap);

	Dx12BufferManager::Instance().CreateConBuffer(_dev, size, PrimCBuff, PrimDescHeap);

	_mappedMatrix = nullptr;
	result = _cBuffer->Map(0, nullptr, (void**)&_mappedMatrix);
	_mappedMatrix->world = tm.world;
	_mappedMatrix->wvp = tm.wvp;
	_mappedMatrix->lvp = tm.lvp;

	_PrimMappedMatrix = nullptr;
	result = PrimCBuff->Map(0, nullptr, (void**)&_PrimMappedMatrix);
	_PrimMappedMatrix->world = primtm.world;
	_PrimMappedMatrix->viewproj = primtm.viewproj;
}

// テクスチャ初期化
void Dx12Initer::InitTexture(){
	CreateModelTextures();
	//白テクスチャとモデルテクスチャ作成
	CreateWhiteTexture();
	CreateBluckTexture();
	CreateGradTexture();
	CreateRGBATexture();
}

// コマンド系初期化
void Dx12Initer::InitCommand(){
	// コマンドキューの生成
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.NodeMask = 0;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	auto result = _dev->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue));

	// コマンドアロケーター作成
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAlloc));
	// コマンドリスト作成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAlloc.Get(), nullptr, IID_PPV_ARGS(&_commandList));
}

// スワップチェイン初期化
void Dx12Initer::InitSwapChain(HWND hwnd){
	// SwapChain関係設定
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	auto wsize = Application::Instance().GetSize();
	swapchainDesc.Width = wsize.w;
	swapchainDesc.Height = wsize.h;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// UNORMは正規化の意味
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// SwapChain作成
	auto result = _dxgiFactory->CreateSwapChainForHwnd(
		_commandQueue.Get(),
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(_swapChain.GetAddressOf())
	);
}

// フェンス初期化
void Dx12Initer::InitFense(){
	// フェンスの作成
	auto result = _dev->CreateFence(_fanceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
}

// RTVデスクリプターヒープ初期化
void Dx12Initer::InitDescHeapForRtv(){
	// ディスクリプター作成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;			// レンダーターゲットビュー
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NumDescriptors = 2;							// 表画面と裏画面分
	descHeapDesc.NodeMask = 0;
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	// ディスクリプターハンドルの取得
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescH(_rtvDescHeap->GetCPUDescriptorHandleForHeapStart());
	// 各ディスクリプタの使用するサイズを計算しとく
	auto heapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CreateRTV(cpuDescH, heapSize, _backBuffer);
}

// ルートシグネチャー系初期化
void Dx12Initer::InitRootSignature(const std::vector<D3D12_ROOT_PARAMETER>& par){
	// ルートシグネチャー作成
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	// 入力アセンブラを指定する
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = par.size();
	rsd.pParameters = par.data();

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MipLODBias = .0f;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &samplerDesc;
	rsd.NumStaticSamplers = 1;

	_rootSignature = Dx12BufferManager::Instance().CreateRC(_dev,rsd);
}

void Dx12Initer::InitPeraRS(const std::vector<D3D12_ROOT_PARAMETER>& par) {
	// ルートシグネチャー作成
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	// 入力アセンブラを指定する
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = par.size();
	rsd.pParameters = par.data();

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MipLODBias = .0f;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &samplerDesc;
	rsd.NumStaticSamplers = 1;

	_peraRootSignature = Dx12BufferManager::Instance().CreateRC(_dev, rsd);
}

// パイプラインステート初期化
void Dx12Initer::InitPiplineState(){
	// インプットレイアウトの設定
	D3D12_INPUT_ELEMENT_DESC layoutDesc[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONENO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	// パイプラインステートオブジェクトを作る
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	// パイプラインそれぞれの情報を設定しておく
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_vsBlob);		// 頂点シェーダ設定
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_psBlob);		// ピクセルシェーダ設定

	// 使わない
	/*gpsDesc.HS;
	gpsDesc.DS;
	gpsDesc.GS;*/

	// 深度ステンシル系
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;	// あとで

	// ラスタライザ系
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザステート設定
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングモードの指定

	// レンダーターゲット系
	gpsDesc.NumRenderTargets = 1;// 1でいい
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// いる

	// ルートシグネチャ系
	gpsDesc.pRootSignature = _rootSignature.Get();			// ルートシグネチャーを入れる

	// その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;			// 0でいい
	gpsDesc.SampleDesc.Count = 1;	// 1でいい
	gpsDesc.SampleDesc.Quality = 0;	// 0でいい
	gpsDesc.SampleMask = UINT_MAX;	// 全部1
	gpsDesc.Flags;					// 多分いる

	// 頂点レイアウト
	gpsDesc.InputLayout.pInputElementDescs = layoutDesc;			// あとで設定絶対指定しないといけない
	gpsDesc.InputLayout.NumElements = _countof(layoutDesc);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// 描画ものの形

	// パイプラインステート作成
	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipeline));
	assert(result == S_OK);
}

void Dx12Initer::InitPeraPiplineState() {
	D3D12_INPUT_ELEMENT_DESC LayoutDesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_peravsBlob);		// 頂点シェーダ設定
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_perapsBlob);		// ピクセルシェーダ設定

	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザステート設定
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングモードの指定

	gpsDesc.NumRenderTargets = 1;// 1でいい
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// いる

	gpsDesc.pRootSignature = _peraRootSignature.Get();			// ルートシグネチャーを入れる

	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;			// 0でいい
	gpsDesc.SampleDesc.Count = 1;	// 1でいい
	gpsDesc.SampleDesc.Quality = 0;	// 0でいい
	gpsDesc.SampleMask = UINT_MAX;	// 全部1
	gpsDesc.Flags;					// 多分いる

									// 頂点レイアウト
	gpsDesc.InputLayout.pInputElementDescs = LayoutDesc;			// あとで設定絶対指定しないといけない
	gpsDesc.InputLayout.NumElements = _countof(LayoutDesc);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// 描画ものの形
																				// パイプラインステート作成
	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_peraPipeline));
	assert(result == S_OK);
}

void Dx12Initer::InitDepthBuffer(){
	// 深度バッファの作成
	Application& app = Application::Instance();
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = app.size.w;
	depthResDesc.Height = app.size.h;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.SampleDesc.Quality = 0;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthheapProp = {};
	depthheapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthheapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthheapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer));

	// 深度バッファビューの作成
	D3D12_DESCRIPTOR_HEAP_DESC _dsvHeapDesc = {};
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc = {};
	_dsvHeap = nullptr;
	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap));
	_dev->CreateDepthStencilView(depthBuffer, &dsvdesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Initer::CreateSRV(ID3D12Resource * res, D3D12_SHADER_RESOURCE_VIEW_DESC desc, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	// SRVデスク設定
	_dev->CreateShaderResourceView(res, &desc,handle);
}

void Dx12Initer::CreateRTV(CD3DX12_CPU_DESCRIPTOR_HANDLE desch, UINT heapsize, std::vector<ID3D12Resource*>& buff) {
	// レンダーターゲットビュー作成
	DXGI_SWAP_CHAIN_DESC swchDesc = {};
	_swapChain->GetDesc(&swchDesc);
	buff.resize(swchDesc.BufferCount);
	for (int i = 0; i < buff.size(); ++i) {
		auto result = _swapChain->GetBuffer(i, IID_PPV_ARGS(&buff[i]));
		_dev->CreateRenderTargetView(buff[i],	// ピクセルを書き込む本体
			nullptr,									// レンダーターゲットビューの仕様
			desch										// ヒープ内の場所
		);
		desch.Offset(heapsize);		// ディスクリプタのサイズ分ポインターをずらす
	}
}

HRESULT Dx12Initer::CreateDescHeap(D3D12_DESCRIPTOR_HEAP_DESC heapdesc, ID3D12DescriptorHeap* & heap) {
	return _dev->CreateDescriptorHeap(&heapdesc, IID_PPV_ARGS(&heap));
}

void Dx12Initer::CreateModelTextures(){
	auto& modeltexPaths = _model->texture_Paths();
	ModelTexBuff.resize(modeltexPaths.size());
	ScratchImage Img = {};
	int idx = 0;
	for (auto& t : modeltexPaths) {
		idx++;
		if (t == "")continue;
		auto texpath = StringToWString(t);
		auto ast = std::count(texpath.begin(), texpath.end(), '*');
		auto count = texpath.rfind(L".");
		auto str = texpath.substr(count + 1, texpath.npos);
		auto result = S_OK;
		if (str == L"tga") {
			result = DirectX::LoadFromTGAFile(texpath.c_str(), &metadata, Img);
		} else if (str == L"dds") {
			result = DirectX::LoadFromDDSFile(texpath.c_str(), 0, &metadata, Img);
		}
		else {
			result = DirectX::LoadFromWICFile(texpath.c_str(), 0, &metadata, Img);
		}
		
		// ヒーププロップ設定
		D3D12_HEAP_PROPERTIES heapprop = {};
		heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask = 1;
		heapprop.VisibleNodeMask = 1;

		// リソースディスクの設定
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Width = metadata.width;
		resDesc.Height = metadata.height;
		resDesc.DepthOrArraySize = metadata.arraySize;
		resDesc.MipLevels = metadata.mipLevels;
		resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ModelTexBuff[idx-1] = Dx12BufferManager::Instance().CreateText(_dev, heapprop, resDesc, ModelTexBuff[idx-1]);
		result = ModelTexBuff[idx-1]->WriteToSubresource(0, nullptr, Img.GetPixels(), metadata.width * 4, Img.GetPixelsSize());
		CreateManagerBuff("Material");
		_commandList->CopyResource(ManagerMaterialBuffer,ModelTexBuff[idx-1]);
	}

	idx = 0;
	palette.resize(_model->ToonData().size());
	for (auto& tp : _model->ToonData()) {
		idx++;
		auto texpath = StringToWString(tp);
		texpath = L"toon/" + texpath;
		auto result = DirectX::LoadFromWICFile(texpath.c_str(), 0, &metadata, Img);
		assert(result == S_OK);

		// ヒーププロップ設定
		D3D12_HEAP_PROPERTIES heapprop = {};
		heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		heapprop.CreationNodeMask = 1;
		heapprop.VisibleNodeMask = 1;

		// リソースディスクの設定
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Width = metadata.width;
		resDesc.Height = metadata.height;
		resDesc.DepthOrArraySize = metadata.arraySize;
		resDesc.MipLevels = metadata.mipLevels;
		resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		palette[idx - 1] = Dx12BufferManager::Instance().CreateText(_dev, heapprop, resDesc, palette[idx - 1]);
		result = palette[idx - 1]->WriteToSubresource(0, nullptr, Img.GetPixels(), metadata.width * 4, Img.GetPixelsSize());
		assert(result == S_OK);
	}

	CloseBarrier("Material");
}

void Dx12Initer::CreateWhiteTexture(){
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	// ヒーププロップ設定
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	// リソースディスクの設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	
	WhiteTexBuff = Dx12BufferManager::Instance().CreateText(_dev, heapprop, resDesc, WhiteTexBuff);
	auto Result = WhiteTexBuff->WriteToSubresource(0, nullptr,data.data(),4*4,4*4*4);
	_commandAlloc->Reset();
	_commandList->Reset(_commandAlloc.Get(), _pipeline.Get());
	CreateManagerBuff("Texture");
	_commandList->CopyResource(ManagerTextureBuffer,WhiteTexBuff);
}

void Dx12Initer::CreateBluckTexture(){
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);

	// ヒーププロップ設定
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	// リソースディスクの設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	BlackTexBuff = Dx12BufferManager::Instance().CreateText(_dev, heapprop, resDesc, BlackTexBuff);
	auto Result = BlackTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4*4*4);
	CreateManagerBuff("Texture");
	_commandList->CopyResource(ManagerTextureBuffer, BlackTexBuff);
	CloseBarrier("Texture");
}

void Dx12Initer::CreateGradTexture() {
	std::vector<Color> data(4 * 256);
	unsigned char Blightness = 255;
	auto it = data.begin();
	for (; it != data.end(); it += 4) {
		std::fill_n(it, 4, Color(Blightness,Blightness,Blightness,0xff));
		--Blightness;
	}

	// ヒーププロップ設定
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	// リソースディスクの設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = 4;
	resDesc.Height = 256;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	GradestionalBuff = Dx12BufferManager::Instance().CreateText(_dev, heapprop, resDesc, GradestionalBuff);
	auto Result = GradestionalBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(Color), data.size() * sizeof(Color));
}

void Dx12Initer::CreateRGBATexture() {
	std::vector<unsigned char> data(1280 * 720 * 4);
	std::fill(data.begin(), data.end(), 0xff);

	// ヒーププロップ設定
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	// リソースディスクの設定
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = 1280;
	resDesc.Height = 720;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// リソースビューデスク設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	RGBAtexBuffer = Dx12BufferManager::Instance().CreateText(_dev, heapprop, resDesc, RGBAtexBuffer);
	auto Result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&RGBAtexBuffer));
	Result = RGBAtexBuffer->WriteToSubresource(0, nullptr, data.data(), 1280 * 4, 1280 * 720 * 4);
	CreateManagerBuff("RGBA");
	_commandList->CopyResource(ManagerRGBABuffer, RGBAtexBuffer);
	CloseBarrier("RGBA");

	D3D12_DESCRIPTOR_HEAP_DESC RGBASDescHeapDesc = {};
	RGBASDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	RGBASDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	RGBASDescHeapDesc.NumDescriptors = 1;
	RGBASDescHeapDesc.NodeMask = 0;

	auto result = CreateDescHeap(RGBASDescHeapDesc, rgbaSDescHeap);

	D3D12_DESCRIPTOR_HEAP_DESC RGBARDescHeapDesc = {};
	RGBARDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RGBARDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RGBARDescHeapDesc.NumDescriptors = 1;
	RGBARDescHeapDesc.NodeMask = 0;

	result = CreateDescHeap(RGBARDescHeapDesc, rgbaRDescHeap);

	CreateSRV(RGBAtexBuffer, srvDesc, rgbaSDescHeap->GetCPUDescriptorHandleForHeapStart());

	_dev->CreateRenderTargetView(RGBAtexBuffer, nullptr, rgbaRDescHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Initer::InitBone() {
	auto& nbone = _model->bones_Data();

	for (int idx = 0; idx < nbone.size(); idx++) {
		auto& b = _model->bones_Data()[idx];
		auto& boneNode = boneMap[b.boneName];
		boneNode.boneIdx = idx;
		boneNode.startPos = b.bonePos;
		boneNode.endPos = nbone[b.tailBoneIndex].bonePos;
	}
	for (auto& b : boneMap) {
		if (nbone[b.second.boneIdx].parentBoneIndex >= nbone.size())continue;
		auto parentName = nbone[nbone[b.second.boneIdx].parentBoneIndex].boneName;
		boneMap[parentName].children.push_back(&b.second);
	}

	XMMATRIX matrix = XMMatrixIdentity();
	auto size = boneMap.size() * sizeof(matrix);
	const auto aligment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	size = (size + aligment - 1)&~(aligment - 1);

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NodeMask = 0;
	desc.NumDescriptors = 1;

	auto result = CreateDescHeap(desc, boneHeap);
	assert(result == S_OK);
	Dx12BufferManager::Instance().CreateConBuffer(_dev, size, boneBuffer, boneHeap);

	mappedbones = nullptr;
	boneMatrices.resize(boneMap.size());

	std::fill(boneMatrices.begin(), boneMatrices.end(), XMMatrixIdentity());

	result = boneBuffer->Map(0, nullptr, (void**)&mappedbones);
	
	/*auto elbow = boneMap["左ひじ"];
	auto vec = XMLoadFloat3(&elbow.startPos);
	boneMatrices[elbow.boneIdx] = XMMatrixTranslationFromVector(XMVectorScale(vec,-1))* XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslationFromVector(vec);

	auto elbowRight = boneMap["右ひじ"];
	auto vecRight = XMLoadFloat3(&elbowRight.startPos);
	boneMatrices[elbowRight.boneIdx] = XMMatrixTranslationFromVector(XMVectorScale(vecRight, -1))* XMMatrixRotationZ(-XM_PIDIV2) * XMMatrixTranslationFromVector(vecRight);*/
}

void Dx12Initer::CreateDsvView() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_DESCRIPTOR_HEAP_DESC DepthDescHeapDesc = {};
	DepthDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DepthDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DepthDescHeapDesc.NumDescriptors = 1;

	auto result = CreateDescHeap(DepthDescHeapDesc, depthView);
	assert(result == S_OK);
	CreateSRV(depthBuffer, srvDesc, depthView->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Initer::InitPrimRS(const std::vector<D3D12_ROOT_PARAMETER>& par) {
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = par.size();
	rsd.pParameters = par.data();

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MipLODBias = .0f;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &samplerDesc;
	rsd.NumStaticSamplers = 1;

	_primRootsignature = Dx12BufferManager::Instance().CreateRC(_dev, rsd);
}

void Dx12Initer::InitPrimPS() {
	D3D12_INPUT_ELEMENT_DESC PrimLayoutDesc[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_primvsBlob);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_primpsBlob);
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザステート設定
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングモードの指定

	gpsDesc.NumRenderTargets = 1;// 1でいい
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// いる

	gpsDesc.pRootSignature = _primRootsignature.Get();			// ルートシグネチャーを入れる

	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;			// 0でいい
	gpsDesc.SampleDesc.Count = 1;	// 1でいい
	gpsDesc.SampleDesc.Quality = 0;	// 0でいい
	gpsDesc.SampleMask = UINT_MAX;	// 全部1
	gpsDesc.Flags;					// 多分いる

									// 頂点レイアウト
	gpsDesc.InputLayout.pInputElementDescs = PrimLayoutDesc;			// あとで設定絶対指定しないといけない
	gpsDesc.InputLayout.NumElements = _countof(PrimLayoutDesc);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// 描画ものの形
																				// パイプラインステート作成
	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_primPipeline));
	assert(result == S_OK);
}

void Dx12Initer::CreateManagerBuff(const std::string& str)
{
	D3D12_HEAP_PROPERTIES heapprop = {};
	D3D12_RESOURCE_DESC resDesc = {};
	auto Result = S_OK;
	// マテリアル用マネージャーバッファー作成
	if (str == "Material") {
		heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapprop.CreationNodeMask = 1;
		heapprop.VisibleNodeMask = 1;

		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Width = metadata.width;
		resDesc.Height = metadata.height;
		resDesc.DepthOrArraySize = metadata.arraySize;
		resDesc.MipLevels = metadata.mipLevels;
		resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		/*_commandAlloc->Reset();
		_commandList->Reset(_commandAlloc.Get(), _pipeline.Get());*/

		Result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&ManagerMaterialBuffer));
		assert(Result == S_OK);
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ManagerMaterialBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	}
	// テクスチャ用マネージャーバッファー作成
	else if (str == "Texture") {

		heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapprop.CreationNodeMask = 1;
		heapprop.VisibleNodeMask = 1;

		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Width = 4;
		resDesc.Height = 4;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		Result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&ManagerTextureBuffer));
		assert(Result == S_OK);
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ManagerTextureBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	}
	// RGBAテクスチャ用マネージャーバッファー作成
	else if (str == "RGBA") {
		heapprop.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapprop.CreationNodeMask = 1;
		heapprop.VisibleNodeMask = 1;

		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Width = 1280;
		resDesc.Height = 720;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		Result = _dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&ManagerRGBABuffer));
		assert(Result == S_OK);
		_commandAlloc->Reset();
		_commandList->Reset(_commandAlloc.Get(), _peraPipeline.Get());
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ManagerRGBABuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	}
}

void Dx12Initer::CloseBarrier(const std::string& str)
{
	if (str == "Material") {
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ManagerMaterialBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
	}
	else if (str == "Texture") {
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ManagerTextureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
	}
	else if (str == "RGBA") {
		_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ManagerRGBABuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));
	}
	_commandList->Close();
	ExecuteCommand();
	WaitExecute();
}

void Dx12Initer::InitShadowRS(const std::vector<D3D12_ROOT_PARAMETER>& par){
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = par.size();
	rsd.pParameters = par.data();

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MipLODBias = .0f;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	rsd.pStaticSamplers = &samplerDesc;
	rsd.NumStaticSamplers = 1;

	ShadowRootsignature = Dx12BufferManager::Instance().CreateRC(_dev, rsd);
}

void Dx12Initer::InitShadowPSO(){
	D3D12_INPUT_ELEMENT_DESC ShadowLayoutDesc[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONENO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(ShadowVsBlob);
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;


	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザステート設定
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングモードの指定

	gpsDesc.pRootSignature = ShadowRootsignature.Get();			// ルートシグネチャーを入れる

	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;			// 0でいい
	gpsDesc.SampleDesc.Count = 1;	// 1でいい
	gpsDesc.SampleDesc.Quality = 0;	// 0でいい
	gpsDesc.SampleMask = UINT_MAX;	// 全部1
	gpsDesc.Flags;					// 多分いる

									// 頂点レイアウト
	gpsDesc.InputLayout.pInputElementDescs = ShadowLayoutDesc;			// あとで設定絶対指定しないといけない
	gpsDesc.InputLayout.NumElements = _countof(ShadowLayoutDesc);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;		// 描画ものの形
																				// パイプラインステート作成
	auto result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&ShadowPipelineState));
	assert(result == S_OK);
}

void Dx12Initer::CreateShadowMap(){
	Application& app = Application::Instance();
	auto wsize = app.GetSize();
	auto size = RoundupPowerOf2(max(wsize.w, wsize.h));

	// DSVヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;
	auto result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ShadowDSV));
	assert(result == S_OK);

	// SRVヒープ作成
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ShadowSRV));
	assert(result == S_OK);

	// 構造体の宣言
	D3D12_HEAP_PROPERTIES HeapProperties = {};
	D3D12_RESOURCE_DESC resource_desc = {};
	D3D12_CLEAR_VALUE ClearValue = {};

	// ヒーププロパティ作成
	HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.VisibleNodeMask = 0;
	// リソースデスク設定
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resource_desc.Width = size;
	resource_desc.Height = size;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 0;
	resource_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	// クリアバリュー設定
	ClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	ClearValue.DepthStencil.Depth = 1.f;
	ClearValue.DepthStencil.Stencil = 0;
	result = _dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &resource_desc, 
		D3D12_RESOURCE_STATE_GENERIC_READ, &ClearValue,
		IID_PPV_ARGS(&ShadowBuffer));
	assert(result == S_OK);

	// DSVビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2D.MipSlice = 0;
	_dev->CreateDepthStencilView(ShadowBuffer, &dsvDesc, ShadowDSV->GetCPUDescriptorHandleForHeapStart());
	// SRVビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC ResViewDesc = {};
	ResViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	ResViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	ResViewDesc.Texture2D.MipLevels = 1;
	ResViewDesc.Texture2D.MostDetailedMip = 0;
	ResViewDesc.Texture2D.PlaneSlice = 0;
	ResViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_dev->CreateShaderResourceView(ShadowBuffer, &ResViewDesc, ShadowSRV->GetCPUDescriptorHandleForHeapStart());
}

size_t Dx12Initer::RoundupPowerOf2(size_t size)
{
	size_t bit = 0x80000000;
	for (size_t i = 31; i >= 0; --i) {
		if (size&bit)break;
		bit >>= 1;
	}
	return bit + (bit%size);
}

void Dx12Initer::InitMaterialTest(){
	auto size = sizeof(Material);
	const auto aligment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	size = (size + aligment - 1)&~(aligment - 1);
	auto& ToonTex = _model->ToonData();
	_materials.resize(_model->materials_Data().size());
	_materialBuffers.resize(_materials.size());

	for (int i = 0; i < _materials.size(); i++) {
		_materials[i].diffuse.x = _model->materials_Data()[i].diffuse_color.x;
		_materials[i].diffuse.y = _model->materials_Data()[i].diffuse_color.y;
		_materials[i].diffuse.z = _model->materials_Data()[i].diffuse_color.z;
		_materials[i].diffuse.w = _model->materials_Data()[i].diffuse_color.w;

		_materials[i].Specular.x = _model->materials_Data()[i].specular_color[0];
		_materials[i].Specular.y = _model->materials_Data()[i].specular_color[1];
		_materials[i].Specular.z = _model->materials_Data()[i].specular_color[2];

		_materials[i].ambient.x = _model->materials_Data()[i].mirror_color[0];
		_materials[i].ambient.y = _model->materials_Data()[i].mirror_color[1];
		_materials[i].ambient.z = _model->materials_Data()[i].mirror_color[2];
	}

	int midx = 0;

	for (auto& matbuff : _materialBuffers) {
		auto result = _dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&matbuff)
		);
		Material* material = nullptr;
		result = matbuff->Map(0, nullptr, (void**)&material);
		material->diffuse.x = _materials[midx].diffuse.x;
		material->diffuse.y = _materials[midx].diffuse.y;
		material->diffuse.z = _materials[midx].diffuse.z;
		material->diffuse.w = _materials[midx].diffuse.w;

		material->Specular.x = _materials[midx].Specular.x;
		material->Specular.y = _materials[midx].Specular.y;
		material->Specular.z = _materials[midx].Specular.z;
		material->Specular.w = _materials[midx].Specular.w;

		material->ambient.x = _materials[midx].ambient.x;
		material->ambient.y = _materials[midx].ambient.y;
		material->ambient.z = _materials[midx].ambient.z;
		material->ambient.w = _materials[midx].ambient.w;

		matbuff->Unmap(0, nullptr);
		++midx;
	}

	D3D12_DESCRIPTOR_HEAP_DESC GradDesc = {};
	GradDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	GradDesc.NodeMask = 0;
	GradDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	GradDesc.NumDescriptors = _materials.size() *3;
	auto result = CreateDescHeap(GradDesc, GradDescHeap);
	assert(result == S_OK);

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = _materials.size() * 4;
	result = CreateDescHeap(desc, _materialHeaps);
	assert(result == S_OK);

	auto handle = _materialHeaps->GetCPUDescriptorHandleForHeapStart();
	auto Gradhandle = GradDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto IncrimentSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	for (int i = 0; i < _materialBuffers.size(); i++) {
		// コンスタントバッファ作成
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.SizeInBytes = size;
		cbvDesc.BufferLocation = _materialBuffers[i]->GetGPUVirtualAddress();
		_dev->CreateConstantBufferView(&cbvDesc, handle);

		handle.ptr += IncrimentSize;
		auto path = _model->texture_Paths();
		auto dotcount = path[i].rfind(".");
		auto textpath = path[i].substr(dotcount + 1, path[i].npos);

		ID3D12Resource* ModelTexBuffer;
		ID3D12Resource* WhiteTexBuffer;
		ID3D12Resource* BlackTexBuffer;

		// テクスチャ作成
		ModelTexBuffer = WhiteTexBuff;
		WhiteTexBuffer = WhiteTexBuff;
		BlackTexBuffer = BlackTexBuff;
		GradestionalBuffer = GradestionalBuff;
		/*if (_model->materials_Data()[i].toon_index != 0) {
			GradestionalBuffer = palette[_model->materials_Data()[i].toon_index];
		} else {
			if (textpath == "spa") {
				BlackTexBuffer = ModelTexBuff[i];
			} else if (textpath == "sph") {
				WhiteTexBuffer = ModelTexBuff[i];
			} else if (strlen(_model->materials_Data()[i].texture_file_name) > 0) {
				ModelTexBuffer = ModelTexBuff[i];
			}
		}*/

		if (textpath == "spa") {
			BlackTexBuffer = ModelTexBuff[i];
		} else if (textpath == "sph") {
			WhiteTexBuffer = ModelTexBuff[i];
		} else if (strlen(_model->materials_Data()[i].texture_file_name) > 0) {
			ModelTexBuffer = ModelTexBuff[i];
		} else {
			GradestionalBuffer = palette[_model->materials_Data()[i].toon_index];
		}

		CreateSRV(ModelTexBuffer, srvDesc, handle);
		handle.ptr += IncrimentSize;
		CreateSRV(WhiteTexBuffer, srvDesc, handle);
		handle.ptr += IncrimentSize;
		CreateSRV(BlackTexBuffer, srvDesc, handle);
		handle.ptr += IncrimentSize;
		CreateSRV(GradestionalBuffer, srvDesc, Gradhandle);
		Gradhandle.ptr += IncrimentSize;
	}
}

// 子ボーンへ親ボーンの回転行列をかける
void Dx12Initer::RecursiveMatrixMultiply(BoneNode& node,XMMATRIX& inmat) {
	boneMatrices[node.boneIdx] *= inmat;
	for (auto& cnode : node.children) {
		RecursiveMatrixMultiply(*cnode,boneMatrices[node.boneIdx]);
	}
}

void Dx12Initer::RotateBoneA(const char * bonename, const DirectX::XMFLOAT4 & q) {
	auto& bonenode = boneMap[bonename];
	auto vec = XMLoadFloat3(&bonenode.startPos);
	auto quaternion = XMLoadFloat4(&q);
	boneMatrices[bonenode.boneIdx] = XMMatrixTranslationFromVector(XMVectorScale(vec, -1))*
		XMMatrixRotationQuaternion(quaternion)*
		XMMatrixTranslationFromVector(vec);
}

// ボーン回転
void Dx12Initer::RotateBoneB(const char * bonename,const DirectX::XMFLOAT4 & q,const DirectX::XMFLOAT4& q2, float t) {
	auto& bonenode = boneMap[bonename];
	auto vec = XMLoadFloat3(&bonenode.startPos);
	auto quaternion = XMLoadFloat4(&q);
	auto quaternion2 = XMLoadFloat4(&q2);
	boneMatrices[bonenode.boneIdx] = XMMatrixTranslationFromVector(XMVectorScale(vec, -1))*
		XMMatrixRotationQuaternion(XMQuaternionSlerp(quaternion, quaternion2,t))*
		XMMatrixTranslationFromVector(vec);
}

void Dx12Initer::AnimationUpDate(int frameno) {
	for (auto& boneanim : _model->animation_Data()) {
		auto& keyFrameVec = boneanim.second;
		std::sort(keyFrameVec.begin(), keyFrameVec.end(), [](KeyFrame& a, KeyFrame& b) {return a.frameNo < b.frameNo; });
		auto frameIt = std::find_if(keyFrameVec.rbegin(), keyFrameVec.rend(), [frameno](const KeyFrame& k) { return k.frameNo <= frameno; });
		if (frameIt == keyFrameVec.rend())continue;
		auto nextIt = frameIt.base();
		if (nextIt == keyFrameVec.end()) {
			RotateBoneA(boneanim.first.c_str(), frameIt->quaternion);
		} else {
			float a = frameIt->frameNo;
			float b = nextIt->frameNo;
			float t = (static_cast<float>(frameno) - a) / (b - a);
			t = GetBezierYValueFromXWithBisection(t, nextIt->bz1, nextIt->bz2);
			RotateBoneB(boneanim.first.c_str(), frameIt->quaternion,nextIt->quaternion,t);
		}
	}

	XMMATRIX rootmat = XMMatrixIdentity();
	RecursiveMatrixMultiply(boneMap["センター"], rootmat);
}

float Dx12Initer::GetBezierYValueFromXWithBisection(float x, const DirectX::XMFLOAT2 & a, const DirectX::XMFLOAT2 & b, const unsigned int limit) {
	if (a.x == a.y&&b.x == b.y)return x;
	float t = x;
	float k0 = 1 + 3 * a.x - 3 * b.x;
	float k1 = 3 * b.x - 6 * a.x;
	float k2 = 3 * a.x;

	const float epsilon = 0.0005f;

	for (int i = 0; i < limit; i++) {
		float r = (1 - t);
		float ft = (t*t*t)*k0 + t * t*k1 + t * k2 - x;
		if (ft <= epsilon && ft >= -epsilon)break;
		float fdt = (3 * t*t*k0 + 2 * t*k1 + k2);
		if (fdt == 0)break;
		t = t - ft / fdt;
	}
	float r = (1 - t);
	return 3 * r*r*t*a.y + 3 * r*t*t*b.y + t * t*t;
}

Dx12Initer::Dx12Initer(HINSTANCE h, HWND hwnd){
	// デバックレイヤを有効にする
	ID3D12Debug* DebugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController)))) {
		DebugController->EnableDebugLayer();
		DebugController->Release();
	}

	// ファクトリ初期化
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	// フューチャーレベル設定
	InitFeatureLev();
	// グラフィックスアダプタ設定
	InitAdapter();
	// コマンド系初期化
	InitCommand();
	// スワップチェイン初期化
	InitSwapChain(hwnd);
	//RTV系デスクリプターヒープ初期化
	InitDescHeapForRtv();

	// フェンス初期化
	InitFense();

	// ビューポート情報設定
	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = Application::Instance().GetSize().w;
	_viewPort.Height = Application::Instance().GetSize().h;
	_viewPort.MinDepth = 0.0f;
	_viewPort.MaxDepth = 1.0f;

	ShadowViewPort.TopLeftX = 0;
	ShadowViewPort.TopLeftY = 0;
	ShadowViewPort.Width = 2048;
	ShadowViewPort.Height = 2048;
	ShadowViewPort.MinDepth = 0.f;
	ShadowViewPort.MaxDepth = 1.f;

	// シザーレクト情報設定
	_rect.left = 0;
	_rect.right = Application::Instance().GetSize().w;
	_rect.top = 0;
	_rect.bottom = Application::Instance().GetSize().h;

	ShadowRect.left = 0;
	ShadowRect.right = 2048;
	ShadowRect.top = 0;
	ShadowRect.bottom = 2048;


	_model.reset(new PMDModel("model/miku/初音ミク.pmd", "モーション/ヤゴコロダンス.vmd"));
	//_model.reset(new PMDModel("model/Latmiku/Lat式ミクVer2.31_Normalエッジ無し専用.pmd"));
	//_model.reset(new PMDModel("model/博麗霊夢/reimu_F01.pmd"));
	//_model.reset(new PMDModel("model/hibiki/我那覇響v1.pmd"));

	//_fbxModel.reset(new FBXModel("char.fbx"));
	InitVertices();		// 頂点情報初期化関数
	InitShader();		// シェーダー系初期化
	InitTexture();		// テクスチャ系初期化
	InitMaterialTest();
	InitConstants();	// コンスタンス系初期化
	InitDepthBuffer();	// 深度バッファ初期化
	InitBone();
	CreateDsvView();
	CreateShadowMap();
}

Dx12Initer::~Dx12Initer(){
}

void Dx12Initer::Update(){
	static float _angle = 0.f;
	static XMFLOAT3 _eye = XMFLOAT3(0.f, 20.f, -15.f);
	static XMFLOAT3 _tag = XMFLOAT3(0.f, 10.f, 0.f);
	unsigned char KeyState[256];
	if (GetKeyboardState(KeyState)) {
		if (KeyState[VK_UP] & 0x80) {
			_eye.y -= 0.1f;
			_tag.y -= 0.1f;
		}
		if (KeyState[VK_DOWN] & 0x80) {
			_eye.y += 0.1f;
			_tag.y += 0.1f;
		}
		if (KeyState[VK_RIGHT] & 0x80) {
			_eye.x += 0.1f;
			_tag.x += 0.1f;
		}
		if (KeyState[VK_LEFT] & 0x80) {
			_eye.x -= 0.1f;
			_tag.x -= 0.1f;
		}
		if (KeyState[*"W"] & 0x80) {
			_eye.z += 0.1f;
		}
		if (KeyState[*"S"] & 0x80) {
			_eye.z -= 0.1f;
		}
		if (KeyState[*"Z"] & 0x80) {
			_angle -= 0.1f;
		}
		if (KeyState[*"X"] & 0x80) {
			_angle += 0.1f;
		}
	}

	///////////////////////////////////////////////////////
	// シャドウマップ用
	auto result = _commandAlloc->Reset();
	_commandList->Reset(_commandAlloc.Get(), ShadowPipelineState.Get());
	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ShadowBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	_commandList->SetGraphicsRootSignature(ShadowRootsignature.Get());
	_commandList->RSSetViewports(1, &ShadowViewPort);
	_commandList->RSSetScissorRects(1, &ShadowRect);
	_commandList->IASetVertexBuffers(0, 1, &vbView);
	_commandList->IASetIndexBuffer(&ibView);
	_commandList->OMSetRenderTargets(0, nullptr, false, &ShadowDSV->GetCPUDescriptorHandleForHeapStart());
	_commandList->ClearDepthStencilView(ShadowDSV->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	static float angle = 0.f;
	Application& app = Application::Instance();
	auto wsize = app.GetSize();
	static XMFLOAT3 eye(0.f, 20.f, -15.f);
	static XMFLOAT3 _target(0.f, 10.f, 0.f);
	XMFLOAT3 _up(0, 1, 0);
	XMVECTOR _veye = XMLoadFloat3(&eye);
	XMVECTOR _vtarget = XMLoadFloat3(&_target);
	XMVECTOR _vup = XMLoadFloat3(&_up);
	XMFLOAT3 lightposition(15, 35, -15);
	XMVECTOR vlightpos = XMLoadFloat3(&Normalize(lightposition));
	auto lightpos = vlightpos * Magnitude(_target - _eye);

	tm.wvp = XMMatrixRotationY(angle);
	tm.wvp *= XMMatrixLookAtLH(_veye, _vtarget, _vup)*
		XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(wsize.w) / static_cast<float>(wsize.h), 0.1f, 300.f);
	_mappedMatrix->wvp = tm.wvp;
	primtm.lvp = tm.lvp = XMMatrixLookAtLH(lightpos, _vtarget, _vup) * XMMatrixOrthographicLH(40, 40, 0.1f, 300.f);
	primtm.world = XMMatrixRotationY(angle);
	primtm.viewproj = XMMatrixLookAtLH(_veye, _vtarget, _vup)*
		XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(wsize.w) / static_cast<float>(wsize.h), 0.1f, 300.f);
	tm.world = XMMatrixRotationY(angle);
	_mappedMatrix->world = tm.world;
	_mappedMatrix->lvp = tm.lvp;
	_PrimMappedMatrix->world = primtm.world;
	_PrimMappedMatrix->viewproj = primtm.viewproj;
	_PrimMappedMatrix->lvp = primtm.lvp;

	_commandList->SetDescriptorHeaps(1, &_rgstDescHeap);
	_commandList->SetGraphicsRootDescriptorTable(0, _rgstDescHeap->GetGPUDescriptorHandleForHeapStart());

	_commandList->SetDescriptorHeaps(1, &boneHeap);
	_commandList->SetGraphicsRootDescriptorTable(2, boneHeap->GetGPUDescriptorHandleForHeapStart());

	_commandList->SetDescriptorHeaps(1, &_materialHeaps);
	auto Modelhandle1 = _materialHeaps->GetGPUDescriptorHandleForHeapStart();
	int Modelindex1 = 0;
	const auto incrimentsize1 = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int i = 0; i < _model->materials_Data().size(); i++) {
		_commandList->SetGraphicsRootDescriptorTable(1, Modelhandle1);
		_commandList->DrawIndexedInstanced(_model->materials_Data()[i].face_vert_count, 1, Modelindex1, 0, 0);
		Modelindex1 += _model->materials_Data()[i].face_vert_count;
		Modelhandle1.ptr += incrimentsize1 * 4;
	}

	_commandList->SetDescriptorHeaps(1, &ShadowSRV);
	_commandList->SetGraphicsRootDescriptorTable(3, ShadowSRV->GetGPUDescriptorHandleForHeapStart());

	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ShadowBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
	_commandList->Close();

	ExecuteCommand();
	WaitExecute();
	/////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////
	// プレーン
	_commandAlloc->Reset();
	_commandList->Reset(_commandAlloc.Get(), _primPipeline.Get());
	auto pfidx = _swapChain->GetCurrentBackBufferIndex();
	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_backBuffer[pfidx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	_commandList->SetGraphicsRootSignature(_primRootsignature.Get());
	_commandList->RSSetViewports(1, &_viewPort);
	_commandList->RSSetScissorRects(1, &_rect);
	_commandList->IASetVertexBuffers(0, 1, &PrimVertBuffView);
	_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	auto PrimDescH = rgbaRDescHeap->GetCPUDescriptorHandleForHeapStart();
	float Color[] = { 0.5f,0.5f,0.5f,1.f };
	_commandList->OMSetRenderTargets(1, &PrimDescH, false, &_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	_commandList->ClearRenderTargetView(PrimDescH, Color, 0, nullptr);
	// primitive
	primtm.world = XMMatrixIdentity();//XMMatrixRotationY(angle);
	primtm.viewproj = XMMatrixLookAtLH(_veye, _vtarget, _vup)*
		XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(wsize.w) / static_cast<float>(wsize.h), 0.1f, 300.f);
	_PrimMappedMatrix->world = primtm.world;
	_PrimMappedMatrix->viewproj = primtm.viewproj;

	_commandList->SetDescriptorHeaps(1, &PrimDescHeap);
	_commandList->SetGraphicsRootDescriptorTable(0, PrimDescHeap->GetGPUDescriptorHandleForHeapStart());
	_commandList->SetDescriptorHeaps(1, &ShadowSRV);
	_commandList->SetGraphicsRootDescriptorTable(1, ShadowSRV->GetGPUDescriptorHandleForHeapStart());
	_commandList->DrawInstanced(4, 1, 0, 0);
	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_backBuffer[pfidx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	_commandList->Close();
	ExecuteCommand();
	WaitExecute();
	////////////////////////////////////////////////////

	////////////////////////////////////////////////////
	// モデル
	_commandAlloc->Reset();							// コマンドアロケーターリセット
	_commandList->Reset(_commandAlloc.Get(), _pipeline.Get());			// コマンドリストリセット

	auto bfidx = _swapChain->GetCurrentBackBufferIndex();
	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_backBuffer[bfidx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// ルートシグネチャーをセット
	_commandList->SetGraphicsRootSignature(_rootSignature.Get());

	// ビューポート設定
	_commandList->RSSetViewports(1, &_viewPort);
	// シザーレクト設定
	_commandList->RSSetScissorRects(1, &_rect);
	// 頂点情報をセット
	_commandList->IASetVertexBuffers(0, 1, &vbView);

	// インデックス情報をセット
	_commandList->IASetIndexBuffer(&ibView);
	_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto descH = rgbaRDescHeap->GetCPUDescriptorHandleForHeapStart();		// ハンドル取得
	
	_commandList->OMSetRenderTargets(1, &descH, false,&_dsvHeap->GetCPUDescriptorHandleForHeapStart());			// レンダーターゲット設定

	//_commandList->ClearRenderTargetView(descH, Color, 0, nullptr);			// クリア

	_commandList->ClearDepthStencilView(_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0.f, 0.f, nullptr);

	tm.wvp = XMMatrixRotationY(angle);
	tm.wvp *= XMMatrixLookAtLH(_veye, _vtarget, _vup)*
		XMMatrixPerspectiveFovLH(XM_PIDIV2, static_cast<float>(wsize.w) / static_cast<float>(wsize.h), 0.1f, 300.f);
	_mappedMatrix->wvp = tm.wvp;
	tm.world = XMMatrixRotationY(angle);
	_mappedMatrix->world = tm.world;
	angle = _angle;
	eye = _eye;

	std::fill(boneMatrices.begin(), boneMatrices.end(), XMMatrixIdentity());
	static auto ModellastTime = GetTickCount();
	if (static_cast<float>(GetTickCount() - ModellastTime) > _model->Duration() * 33.33333f) {
		ModellastTime = GetTickCount();
	}
	AnimationUpDate(static_cast<float>(GetTickCount() - ModellastTime) / 33.33333f);
	std::copy(boneMatrices.begin(), boneMatrices.end(), mappedbones);

	_commandList->SetDescriptorHeaps(1,&_rgstDescHeap);
	_commandList->SetGraphicsRootDescriptorTable(0, _rgstDescHeap->GetGPUDescriptorHandleForHeapStart());

	// ボーン用バッファ設定
	_commandList->SetDescriptorHeaps(1, &boneHeap);
	_commandList->SetGraphicsRootDescriptorTable(2, boneHeap->GetGPUDescriptorHandleForHeapStart());

	auto GradModelHandle = GradDescHeap->GetGPUDescriptorHandleForHeapStart();
	_commandList->SetDescriptorHeaps(1, &GradDescHeap);
	_commandList->SetGraphicsRootDescriptorTable(3, GradModelHandle);

	// マテリアル用バッファ設定
	_commandList->SetDescriptorHeaps(1, &_materialHeaps);
	auto Modelhandle = _materialHeaps->GetGPUDescriptorHandleForHeapStart();
	
	int Modelindex = 0;
	const auto incrimentsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int j = 0; j < _model->materials_Data().size();j++) {
		_commandList->SetGraphicsRootDescriptorTable(1, Modelhandle);
		_commandList->DrawIndexedInstanced(_model->materials_Data()[j].face_vert_count, 1, Modelindex, 0, 0);
		Modelindex += _model->materials_Data()[j].face_vert_count;
		Modelhandle.ptr += incrimentsize * 4;
	}

	

	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_backBuffer[bfidx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT ));

	_commandList->Close();		// コマンドリストのクローズ

	ExecuteCommand();
	WaitExecute();
	///////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////
	// ペラポリ
	_commandAlloc->Reset();
	_commandList->Reset(_commandAlloc.Get(), _peraPipeline.Get());

	auto idx = _swapChain->GetCurrentBackBufferIndex();
	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_backBuffer[idx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	_commandList->SetGraphicsRootSignature(_peraRootSignature.Get());

	_commandList->RSSetViewports(1, &_viewPort);

	_commandList->RSSetScissorRects(1, &_rect);

	_commandList->IASetVertexBuffers(0, 1, &peraVBV);

	_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	auto peradescH = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();		// ハンドル取得
	auto peraHeapSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	peradescH.ptr += peraHeapSize * idx;
	float peraColor[] = { 0.5f,0.5f,0.7f,1.f };

	_commandList->OMSetRenderTargets(1, &peradescH, false, &_dsvHeap->GetCPUDescriptorHandleForHeapStart());			// レンダーターゲット設定

	_commandList->ClearRenderTargetView(peradescH, peraColor, 0, nullptr);			// クリア

	_commandList->SetDescriptorHeaps(1, &rgbaSDescHeap);
	_commandList->SetGraphicsRootDescriptorTable(0, rgbaSDescHeap->GetGPUDescriptorHandleForHeapStart());

	_commandList->SetDescriptorHeaps(1, &depthView);
	_commandList->SetGraphicsRootDescriptorTable(1, depthView->GetGPUDescriptorHandleForHeapStart());

	_commandList->DrawInstanced(4, 1, 0, 0);

	_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_backBuffer[idx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	_commandList->Close();

	ExecuteCommand();
	WaitExecute();
	/////////////////////////////////////////////////////////

	_swapChain->Present(0, 0);
}

void Dx12Initer::ExecuteCommand(){
	ID3D12CommandList* cmdList[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, cmdList);
	_commandQueue->Signal(_fence.Get(), ++_fanceValue);
}

void Dx12Initer::WaitExecute(){
	while (_fence->GetCompletedValue() < _fanceValue)
		;
}
