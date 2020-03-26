#include "Plane.h"
#include <memory>
#include "d3dx12.h"

Plane::Plane() {
	vertices.push_back(PrimitiveVertex(XMFLOAT3(-10.f,-0.2f,10.f), XMFLOAT3(0, 1, 0), XMFLOAT2(0,0)));
	vertices.push_back(PrimitiveVertex(XMFLOAT3(10.f, -0.2f, 10.f), XMFLOAT3(0, 1, 0), XMFLOAT2(1,0)));
	vertices.push_back(PrimitiveVertex(XMFLOAT3(-10.f, -0.2f, -10.f), XMFLOAT3(0, 1, 0), XMFLOAT2(0,1)));
	vertices.push_back(PrimitiveVertex(XMFLOAT3(10.f, -0.2f, -10.f), XMFLOAT3(0, 1, 0), XMFLOAT2(1,1)));
}

Plane::~Plane() {
}

void Plane::VertexBuffer(const ComPtr<ID3D12Device>& dev) {
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&PlaneVertexBuff));

	PlaneVertView.BufferLocation = PlaneVertexBuff->GetGPUVirtualAddress();
	PlaneVertView.SizeInBytes = sizeof(vertices);
	PlaneVertView.StrideInBytes = sizeof(PrimitiveVertex);

	D3D12_RANGE peravertexrange = { 0,0 };
	PrimitiveVertex* planebuffptr = nullptr;
	result = PlaneVertexBuff->Map(0, &peravertexrange, (void**)&planebuffptr);
	std::copy(std::begin(vertices), std::end(vertices), planebuffptr);
	PlaneVertexBuff->Unmap(0, &peravertexrange);
}

void Plane::Draw(const ComPtr<ID3D12GraphicsCommandList>& cmdList) {
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &PlaneVertView);
	cmdList->DrawInstanced(4, 1, 0, 0);
}
