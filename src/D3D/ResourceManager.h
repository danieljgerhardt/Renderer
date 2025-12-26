#pragma once

#include <unordered_map>

#include "Support/WinInclude.h"
#include "D3D/DXContext.h"
#include "D3D/Pipeline/RenderPipeline.h"

#include "D3D/Texture.h"
#include "D3D/Buffer/VertexBuffer.h"
#include "D3D/Buffer/IndexBuffer.h"
#include "D3D/Buffer/StructuredBuffer.h"
#include "Scene/Util/Mesh.h"
#include "D3D/DescriptorHeap.h"

enum ResourceType {
	TEXTURE = 0,
	VERTEX_BUFFER,
	INDEX_BUFFER,
	STRUCTURED_BUFFER,
	MESH,
	DESCRIPTOR_HEAP
};

struct ResourceHandle {
	ResourceType type;
	UINT id;
};

class ResourceManager {
public:
	static ResourceManager& get(DXContext* context) {
		static ResourceManager instance(context);
		return instance;
	}

	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	ResourceHandle createTexture(RenderPipeline* pipeline, UINT width, UINT height, std::vector<unsigned char> imageData, TextureType type);
	ResourceHandle createTextureFromFile(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, TextureType type);
	ResourceHandle createVertexBuffer(RenderPipeline* pipeline, std::vector<Vertex>& vertexData, UINT vertexDataSize, UINT vertexDataStride);
	ResourceHandle createIndexBuffer(RenderPipeline* pipeline, std::vector<UINT>& indexData, UINT indexDataSize);
	ResourceHandle createStructuredBuffer(RenderPipeline* pipeline, void* data, UINT elementCount, UINT elementSize);
	ResourceHandle createMesh(RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix, MeshData meshData);
	ResourceHandle createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	Texture* getTexture(ResourceHandle handle) {
		return textures[handle.id].get();
	}

	VertexBuffer* getVertexBuffer(ResourceHandle handle) {
		return vertexBuffers[handle.id].get();
	}

	IndexBuffer* getIndexBuffer(ResourceHandle handle) {
		return indexBuffers[handle.id].get();
	}

	StructuredBuffer* getStructuredBuffer(ResourceHandle handle) {
		return structuredBuffers[handle.id].get();
	}

	Mesh* getMesh(ResourceHandle handle) {
		return meshes[handle.id].get();
	}

	DescriptorHeap* getDescriptorHeap(ResourceHandle handle) {
		return descriptorHeaps[handle.id].get();
	}

	void releaseAllResources();

private:
	ResourceManager(DXContext* context) : context(context) {}
	~ResourceManager() { releaseAllResources(); }

	DXContext* context;

	std::unordered_map<UINT, std::unique_ptr<Texture>> textures;
	UINT textureCount = 0;

	std::unordered_map<UINT, std::unique_ptr<VertexBuffer>> vertexBuffers;
	UINT vertexBufferCount = 0;

	std::unordered_map<UINT, std::unique_ptr<IndexBuffer>> indexBuffers;
	UINT indexBufferCount = 0;

	std::unordered_map<UINT, std::unique_ptr<StructuredBuffer>> structuredBuffers;
	UINT structuredBufferCount = 0;

	std::unordered_map<UINT, std::unique_ptr<Mesh>> meshes;
	UINT meshCount = 0;

	std::unordered_map<UINT, std::unique_ptr<DescriptorHeap>> descriptorHeaps;
	UINT descriptorHeapCount = 0;
};