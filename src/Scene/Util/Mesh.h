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
#include "D3D/Texture.h"

//data passed to a mesh for construction
struct MeshData {
	UINT numTriangles{ 0 };
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};

class Mesh {
public:
	Mesh() = delete;

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	Mesh(DXContext* context, RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix, MeshData meshData);

	~Mesh();

	D3D12_INDEX_BUFFER_VIEW* getIBV();
	D3D12_VERTEX_BUFFER_VIEW* getVBV();

	XMFLOAT4X4* getModelMatrix();

	void releaseResources();

	UINT getNumTriangles();

	//TODO - use smart pointers or resource manager to cleanly transfer ownership
	void assignTexture(Texture* tex) {
		TextureType type = tex->getType();
		if (type > NUM_TEXTURE_TYPES - 1) {
			std::cerr << "Error: Unsupported texture type assigned to mesh.\n";
		}
		textures[type] = tex;
	}

	Texture* getTexture(TextureType type) {
		if (type > NUM_TEXTURE_TYPES - 1) {
			return nullptr;
		}
		return textures[type];
	}

private:
	std::vector<Vertex> vertices;
	std::vector<XMFLOAT3> vertexPositions;
	std::vector<unsigned int> indices;
	UINT numTriangles;

	VertexBuffer* vertexBuffer;
	IndexBuffer* indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;

	XMFLOAT4X4 modelMatrix;

	std::vector<Texture*> textures;
};