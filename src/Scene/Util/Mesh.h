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
	std::vector<XMFLOAT3> vertexPositions; //TODO - may be obsolete once transitioned to using full vertex data and not just positions
	std::vector<unsigned int> indices;
};

class Mesh {
public:
	Mesh() = delete;

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	Mesh(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix, MeshData meshData);

	~Mesh();

	D3D12_INDEX_BUFFER_VIEW* getIBV();
	D3D12_VERTEX_BUFFER_VIEW* getVBV();

	XMFLOAT4X4* getModelMatrix();

	void releaseResources();

	UINT getNumTriangles();

	//TODO - use smart pointers or resource manager to cleanly transfer ownership
	void assignTextures(Texture diffuseTex, Texture normalTex, Texture metallicRoughnessTex, Texture emissiveTex) {
		textures.reserve(4);
		textures.push_back(std::move(diffuseTex));
		textures.push_back(std::move(normalTex));
		textures.push_back(std::move(metallicRoughnessTex));
		textures.push_back(std::move(emissiveTex));
	}

	Texture& getDiffuseTexture() {
		return textures[0];
	}

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

	std::vector<Texture> textures;
};