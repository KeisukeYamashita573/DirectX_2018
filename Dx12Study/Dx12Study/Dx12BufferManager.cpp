#include "Dx12BufferManager.h"
#include <cassert>


ComPtr<ID3D12RootSignature> Dx12BufferManager::CreateRC(ComPtr<ID3D12Device>& dev, const D3D12_ROOT_SIGNATURE_DESC& rs ){
	ID3DBlob* rootSignatureBlob = nullptr;	// ���[�g�V�O�l�`���[����邽�߂̍ޗ�
	ID3DBlob* error = nullptr;				// �G���[���o�����̑Ώ�
	ComPtr<ID3D12RootSignature> _rootSignature;
	// �V�O�l�`���[�̍쐬
	auto result = D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &error);
	assert(result == S_OK);
	// ���[�g�V�O�l�`���[�{�̂̍쐬
	result = dev->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
	assert(result == S_OK);
	return _rootSignature;
}

void Dx12BufferManager::CreateConBuffer(ComPtr<ID3D12Device>& dev, const unsigned long long& size, ID3D12Resource* & cbuffer, ID3D12DescriptorHeap* & desch){
	auto result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cbuffer)
	);

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = cbuffer->GetGPUVirtualAddress();
	desc.SizeInBytes = size;
	auto handle = desch->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&desc, handle);
}

ID3D12Resource* Dx12BufferManager::CreateText(ComPtr<ID3D12Device>& dev, const D3D12_HEAP_PROPERTIES& pro, const D3D12_RESOURCE_DESC& redes, ID3D12Resource* & texbuffer){
	// �e�N�X�`���o�b�t�@�쐬
	auto result = dev->CreateCommittedResource(
		&pro,
		D3D12_HEAP_FLAG_NONE,
		&redes,
		D3D12_RESOURCE_STATE_GENERIC_READ,//rtv�ɃZ�b�g����
		nullptr,
		IID_PPV_ARGS(&texbuffer));
	return texbuffer;
}

Dx12BufferManager::Dx12BufferManager(){
}


Dx12BufferManager::~Dx12BufferManager(){
}
