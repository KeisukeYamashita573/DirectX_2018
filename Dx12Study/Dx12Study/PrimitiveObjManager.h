#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <wrl.h>

#pragma comment(lib,"d3d12.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct PrimitiveVertex {
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT3 color;
	XMFLOAT2 uv;

	PrimitiveVertex() {
		pos = XMFLOAT3(0, 0, 0);
		normal = XMFLOAT3(0, 0, 0);
		color = XMFLOAT3(0, 0, 0);
		uv = XMFLOAT2(0, 0);
	}

	PrimitiveVertex(const XMFLOAT3& p, const XMFLOAT3& norm, const XMFLOAT2& coord) {
		pos = p;
		normal = norm;
		uv = coord;
	}

	PrimitiveVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
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

class PrimitiveObjManager {
	ID3DBlob* PrimVsBlob = nullptr;
	ID3DBlob* PrimPsBlob = nullptr;
	ID3D12PipelineState* PrimPipeline;
public:
	PrimitiveObjManager();
	~PrimitiveObjManager();

	virtual void VertexBuffer(const ComPtr<ID3D12Device>& dev);
	void Init(const ComPtr<ID3D12Device>& dev);
	void SetPrimitiveDrawMode(const ComPtr<ID3D12GraphicsCommandList>& cmdList);
	void Crate(const ComPtr<ID3D12Device>& dev);
	virtual void Draw();
};

