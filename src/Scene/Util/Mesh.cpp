#include "Mesh.h"

#include "D3D/ResourceManager.h"

#include "D3D/Pipeline/RayPipeline.h"

Mesh::Mesh(DXContext* context, Pipeline* pipeline, XMFLOAT4X4 p_modelMatrix, MeshData meshData) {
	numTriangles = meshData.numTriangles;
	vertices = meshData.vertices;
	indices = meshData.indices;

    //a mesh will never have an env map
    textures.resize(NUM_TEXTURE_TYPES - 1);

    //if pipeline is raypipeline, populate vertexpositions and vertex buffer with just positions
	if (dynamic_cast<RayPipeline*>(pipeline) != nullptr) {
		vertexPositions.reserve(vertices.size());
		for (const Vertex& vertex : vertices) {
			vertexPositions.push_back(vertex.pos);
		}
		vertexBuffer = ResourceManager::get(context).getVertexBuffer(
			ResourceManager::get(context).createVertexBuffer(
				vertexPositions.data(),
				(UINT)(vertexPositions.size() * sizeof(XMFLOAT3)),
				(UINT)sizeof(XMFLOAT3)
			)
		);
	}
	else {
		vertexBuffer = ResourceManager::get(context).getVertexBuffer(
			ResourceManager::get(context).createVertexBuffer(
				vertices.data(),
				(UINT)(vertices.size() * sizeof(Vertex)),
				(UINT)sizeof(Vertex)
			)
		);
	}

	indexBuffer = ResourceManager::get(context).getIndexBuffer(
		ResourceManager::get(context).createIndexBuffer(
			indices,
			(UINT)(indices.size() * sizeof(UINT))
		)
	);

    modelMatrix = p_modelMatrix;

	ID3D12GraphicsCommandList6* cmdList = pipeline->getCommandList();
    vbv = vertexBuffer->passVertexDataToGPU(*context, cmdList);
    ibv = indexBuffer->passIndexDataToGPU(*context, cmdList);

    //Transition both buffers to their usable states
    D3D12_RESOURCE_BARRIER barriers[2] = {};

    // Vertex buffer barrier
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = vertexBuffer->getVertexBuffer().Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // Index buffer barrier
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = indexBuffer->getIndexBuffer().Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    cmdList->ResourceBarrier(2, barriers);
}

Mesh::~Mesh() {
	releaseResources();
}

D3D12_INDEX_BUFFER_VIEW* Mesh::getIBV() {
    return &ibv;
}

D3D12_VERTEX_BUFFER_VIEW* Mesh::getVBV() {
    return &vbv;
}

XMFLOAT4X4* Mesh::getModelMatrix() {
    return &modelMatrix;
}

void Mesh::releaseResources() {
	//TODO - not needed with resource manager
}

UINT Mesh::getNumTriangles() {
    return numTriangles;
}
