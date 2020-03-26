#pragma once
#include <wrl.h>
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

class Dx12BufferManager{
private:
	Dx12BufferManager();
	Dx12BufferManager(const Dx12BufferManager&) {};
	void operator=(const Dx12BufferManager&) {};
public:
	static Dx12BufferManager& Instance() {
		static Dx12BufferManager instance;
		return instance;
	}
	~Dx12BufferManager();

	ComPtr<ID3D12RootSignature> CreateRC(ComPtr<ID3D12Device>& dev, const D3D12_ROOT_SIGNATURE_DESC& rs);	// ���[�g�V�O�l�`���쐬
	void CreateConBuffer(ComPtr<ID3D12Device>& dev, const unsigned long long& size, ID3D12Resource* & cbuffer, ID3D12DescriptorHeap* & desch);		// �R���X�^���g�o�b�t�@�쐬
	ID3D12Resource* CreateText(ComPtr<ID3D12Device>& dev, const D3D12_HEAP_PROPERTIES& pro, const D3D12_RESOURCE_DESC& redes, ID3D12Resource* & texbuffer);			// �e�N�X�`���쐬
};

