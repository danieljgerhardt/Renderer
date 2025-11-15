#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "Support/WinInclude.h"

#include "D3D/DXContext.h"
#include "D3D/Buffer/VertexBuffer.h"
#include "D3D/Buffer/IndexBuffer.h"
#include "D3D/Buffer/StructuredBuffer.h"

#include "D3D/Pipeline/RenderPipeline.h"

#include "Vertex.h"

//data passed to a mesh for construction
struct MeshData {
	UINT numTriangles;
	std::vector<Vertex> vertices;
	std::vector<XMFLOAT3> vertexPositions;
	std::vector<unsigned int> indices;
};

class Mesh {
public:
	Mesh() = delete;
	Mesh(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix, XMFLOAT3 color, MeshData meshData);

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