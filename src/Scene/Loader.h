#pragma once

#include <string>

#include "Support/WinInclude.h"

#include "Mesh.h"
#include "Vertex.h"


class Loader {
public:
	static std::vector<Mesh> createMeshFromGltf(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, RenderPipeline* pipeline, XMFLOAT4X4 modelMatrix, XMFLOAT3 color);

private:
	static void loadMeshFromObj(std::string fileLocation, MeshData& meshData);
	static void loadMeshFromGltf(std::string fileLocation, std::vector<MeshData>& meshDataVector);
};
