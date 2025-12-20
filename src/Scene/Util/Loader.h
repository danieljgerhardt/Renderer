#pragma once

#include <string>

#include "Support/WinInclude.h"

#include "D3D/ResourceManager.h"

#include "D3D/Texture.h"

#include "Mesh.h"
#include "Vertex.h"

struct GltfData {
	std::vector<Mesh*> meshes;
	std::vector<Texture*> textures;
};

struct GltfConstructionData {
	std::vector<MeshData> meshDataVector;
	std::vector<TextureData> textureDataVector;
};

class Loader {
public:
	static GltfData createMeshFromGltf(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix);
	static Texture createTextureFromFile(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, TextureType type);

private:
	static void loadMeshFromObj(std::string fileLocation, MeshData& meshData);
	static void loadDataFromGltf(std::string fileLocation, GltfConstructionData& gltfConstructionData);
};
