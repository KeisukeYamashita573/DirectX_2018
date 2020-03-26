#pragma once
#include "PrimitiveObjManager.h"


class Plane :
	public PrimitiveObjManager {
	std::vector<PrimitiveVertex> vertices;
	ID3D12Resource* PlaneVertexBuff;
	D3D12_VERTEX_BUFFER_VIEW PlaneVertView;
public:
	Plane();
	~Plane();
	void VertexBuffer(const ComPtr<ID3D12Device>& dev);
	void Draw(const ComPtr<ID3D12GraphicsCommandList>& cmdList);
};

