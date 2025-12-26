#include "ResourceManager.h"

#include "Scene/Util/Loader.h"

ResourceHandle ResourceManager::createTexture(RenderPipeline* pipeline, UINT width, UINT height, std::vector<unsigned char> imageData, TextureType type) {
	std::unique_ptr<Texture> texture = std::make_unique<Texture>(context, pipeline, width, height, imageData, type);
	UINT id = textureCount++;
	textures[id] = std::move(texture);
	return ResourceHandle{ ResourceType::TEXTURE, id };
}

ResourceHandle ResourceManager::createTextureFromFile(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, TextureType type) {
	Texture newTex = Loader::createTextureFromFile(fileLocation, context, cmdList, pipeline, type);
	std::unique_ptr<Texture> texture = std::make_unique<Texture>(std::move(newTex));
	UINT id = textureCount++;
	textures[id] = std::move(texture);
	return ResourceHandle{ ResourceType::TEXTURE, id };
}

ResourceHandle ResourceManager::createVertexBuffer(RenderPipeline* pipeline, std::vector<Vertex>& vertexData, UINT vertexDataSize, UINT vertexDataStride) {
	std::unique_ptr<VertexBuffer> vertexBuffer = std::make_unique<VertexBuffer>(vertexData, vertexDataSize, vertexDataStride);
	UINT id = vertexBufferCount++;
	vertexBuffers[id] = std::move(vertexBuffer);
	return ResourceHandle{ ResourceType::VERTEX_BUFFER, id };
}

ResourceHandle ResourceManager::createIndexBuffer(RenderPipeline* pipeline, std::vector<UINT>& indexData, UINT indexDataSize) {
	std::unique_ptr<IndexBuffer> indexBuffer = std::make_unique<IndexBuffer>(indexData, indexDataSize);
	UINT id = indexBufferCount++;
	indexBuffers[id] = std::move(indexBuffer);
	return ResourceHandle{ ResourceType::INDEX_BUFFER, id };
}

ResourceHandle ResourceManager::createStructuredBuffer(RenderPipeline* pipeline, void* data, UINT elementCount, UINT elementSize) {
	std::unique_ptr<StructuredBuffer> structuredBuffer = std::make_unique<StructuredBuffer>(data, elementCount, elementSize);
	UINT id = structuredBufferCount++;
	structuredBuffers[id] = std::move(structuredBuffer);
	return ResourceHandle{ ResourceType::STRUCTURED_BUFFER, id };
}

ResourceHandle ResourceManager::createMesh(RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix, MeshData meshData) {
	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(context, pipeline, modelMatrix, meshData);
	UINT id = meshCount++;
	meshes[id] = std::move(mesh);
	return ResourceHandle{ ResourceType::MESH, id };
}

ResourceHandle ResourceManager::createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
	std::unique_ptr<DescriptorHeap> descriptorHeap = std::make_unique<DescriptorHeap>(*context, type, numberOfDescriptors, flags);
	UINT id = descriptorHeapCount++;
	descriptorHeaps[id] = std::move(descriptorHeap);
	return ResourceHandle{ ResourceType::DESCRIPTOR_HEAP, id };
}

void ResourceManager::releaseAllResources() {
	for (auto& [id, texture] : textures) {
		texture->releaseResources();
	}
	textures.clear();
	for (auto& [id, vertexBuffer] : vertexBuffers) {
		vertexBuffer->releaseResources();
	}
	vertexBuffers.clear();
	for (auto& [id, indexBuffer] : indexBuffers) {
		indexBuffer->releaseResources();
	}
	indexBuffers.clear();
	for (auto& [id, structuredBuffer] : structuredBuffers) {
		structuredBuffer->releaseResources();
	}
	structuredBuffers.clear();
	for (auto& [id, mesh] : meshes) {
		mesh->releaseResources();
	}
	meshes.clear();
	for (auto& [id, descriptorHeap] : descriptorHeaps) {
		descriptorHeap->releaseResources();
	}
	descriptorHeaps.clear();
}
