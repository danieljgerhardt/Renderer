#include "Mesh.h"

Mesh::Mesh(std::string fileLocation, DXContext* context, ID3D12GraphicsCommandList6* cmdList, 
           RenderPipeline* pipeline, XMFLOAT4X4 p_modelMatrix, XMFLOAT3 p_color) 
    : color(p_color) {
    //TODO - check file type to avoid switching
	//Loader::loadMeshFromObj(fileLocation, numTriangles, vertices, vertexPositions, indices);
	Loader::loadMeshFromGltf(fileLocation, numTriangles, vertices, vertexPositions, indices);
    vertexBuffer = VertexBuffer(vertexPositions, (UINT)(vertices.size() * sizeof(XMFLOAT3)), (UINT)sizeof(XMFLOAT3));
    indexBuffer = IndexBuffer(indices, (UINT)(indices.size() * sizeof(unsigned int)));

    modelMatrix = p_modelMatrix;

    vbv = vertexBuffer.passVertexDataToGPU(*context, cmdList);
    ibv = indexBuffer.passIndexDataToGPU(*context, cmdList);

    //Transition both buffers to their usable states
    D3D12_RESOURCE_BARRIER barriers[2] = {};

    // Vertex buffer barrier
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = vertexBuffer.getVertexBuffer().Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // Index buffer barrier
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = indexBuffer.getIndexBuffer().Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    cmdList->ResourceBarrier(2, barriers);
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
    vertexBuffer.releaseResources();
    indexBuffer.releaseResources();
}

UINT Mesh::getNumTriangles() {
    return numTriangles;
}
