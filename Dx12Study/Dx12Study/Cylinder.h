#pragma once
#include "PrimitiveObjManager.h"
#include <wrl.h>

using Microsoft::WRL::ComPtr;

class Cylinder :
	public PrimitiveObjManager {
public:
	Cylinder();
	~Cylinder();

	void VertexBuffer(const ComPtr<ID3D12Device>& dev);
	void Drow();
};

