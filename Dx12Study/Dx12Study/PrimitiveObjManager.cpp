#include "PrimitiveObjManager.h"
#include <d3dcompiler.h>
#include "d3dx12.h"
#include "Plane.h"
#include "Cylinder.h"

#pragma comment(lib,"d3dcompiler.lib")

PrimitiveObjManager::PrimitiveObjManager() {
}


PrimitiveObjManager::~PrimitiveObjManager() {
}

void PrimitiveObjManager::VertexBuffer(const ComPtr<ID3D12Device>& dev) {
}

void PrimitiveObjManager::Init(const ComPtr<ID3D12Device>& dev) {
	ID3DBlob* error = nullptr;

	// シェーダーコンパイル
	auto result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &PrimVsBlob, &error);
	result = D3DCompileFromFile(L"Primitive.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &PrimPsBlob, &error);

	// デスクリプターテーブル設定
	D3D12_DESCRIPTOR_RANGE PrimDescTblRange = {};
	D3D12_ROOT_PARAMETER PrimRootParam = {};

	PrimDescTblRange.NumDescriptors = 1;
	PrimDescTblRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	PrimDescTblRange.BaseShaderRegister = 0;
	PrimDescTblRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	PrimRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	PrimRootParam.DescriptorTable.NumDescriptorRanges = 1;
	PrimRootParam.DescriptorTable.pDescriptorRanges = &PrimDescTblRange;
	PrimRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// サンプラー設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 1;
	rsd.pParameters = &PrimRootParam;

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

	// ルートシグネチャー系
	ID3DBlob* rootSignatureBlob = nullptr;	// ルートシグネチャーを作るための材料
	ID3DBlob* sigerror = nullptr;				// エラーが出た時の対処
	ComPtr<ID3D12RootSignature> _rootSignature;
	// シグネチャーの作成
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &sigerror);
	assert(result == S_OK);
	// ルートシグネチャー本体の作成
	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
	assert(result == S_OK);

	// パイプライン初期化
	D3D12_INPUT_ELEMENT_DESC layoutDesc[] = {
	{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	{ "COLOR",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	// パイプラインステートオブジェクトを作る
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	// パイプラインそれぞれの情報を設定しておく
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(PrimVsBlob);		// 頂点シェーダ設定
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(PrimPsBlob);		// ピクセルシェーダ設定

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
	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&PrimPipeline));
	assert(result == S_OK);
}

void PrimitiveObjManager::SetPrimitiveDrawMode(const ComPtr<ID3D12GraphicsCommandList>& cmdList) {
	cmdList->SetPipelineState(PrimPipeline);
}

void PrimitiveObjManager::Crate(const ComPtr<ID3D12Device>& dev) {
	Plane plane = Plane();
	//Cylinder cylinder = Cylinder();
	plane.VertexBuffer(dev);
	//cylinder.VertexBuffer(dev);
}

void PrimitiveObjManager::Draw() {

}
