#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Support/WinInclude.h"

#include "D3D/DXContext.h"
#include "D3D/VertexBuffer.h"
#include "D3D/IndexBuffer.h"
#include "D3D/StructuredBuffer.h"

#include "D3D/Pipeline/RenderPipeline.h"

#include "Loader.h"
#include "Vertex.h"

class Mesh {
public:
	Mesh() = delete;
	Mesh(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix, XMFLOAT3 color = {0, 0, 0});

	D3D12_INDEX_BUFFER_VIEW* getIBV();
	D3D12_VERTEX_BUFFER_VIEW* getVBV();

	XMFLOAT4X4* getModelMatrix();

	void releaseResources();

	UINT getNumTriangles();

	XMFLOAT3* getColor() { return &color; }

private:
	std::vector<Vertex> vertices;
	std::vector<XMFLOAT3> vertexPositions;
	std::vector<unsigned int> indices;
	UINT numTriangles;

	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;

	XMFLOAT4X4 modelMatrix;

	XMFLOAT3 color;
};