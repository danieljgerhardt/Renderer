#pragma once

#include "D3D/DXContext.h"

using namespace DirectX;

struct Vertex {
	XMFLOAT3 pos;
	XMFLOAT3 nor;
	XMFLOAT2 uv;
};