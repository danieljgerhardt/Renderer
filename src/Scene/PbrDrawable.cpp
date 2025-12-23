#include "PbrDrawable.h"

PbrDrawable::PbrDrawable(DXContext* context, RenderPipeline* pipeline) : context(context), renderPipeline(pipeline) {
    construct();
}

void PbrDrawable::construct() {
    std::vector<std::string> inputStrings;
    inputStrings.push_back("objs\\Cube\\Cube.gltf");

    XMFLOAT4X4 avocadoModelMatrix;
    XMStoreFloat4x4(&avocadoModelMatrix, XMMatrixMultiply(
        XMMatrixScaling(10.f, 10.f, 10.f),
        XMMatrixTranslation(50.f, 30.f, 30.f)
    ));
    modelMatrices.push_back(avocadoModelMatrix);

    std::string& string = inputStrings.front();
    DirectX::XMFLOAT4X4& m = modelMatrices.front();

    UINT currentMeshIdx = 0;
    for (UINT i = 0; i < inputStrings.size(); i++) {
        std::string& inputString = inputStrings[i];
        gltfData = Loader::createMeshFromGltf((std::filesystem::current_path() / inputString).string(), context, renderPipeline->getCommandList(), renderPipeline, modelMatrices[i]);

        for (Mesh* newMesh : gltfData.meshes) {
            meshes.push_back(newMesh);

            triangleCount += meshes.back()->getNumTriangles();
        }
    }
}

void PbrDrawable::draw(Camera* camera) {
    for (Mesh* m : meshes) {
        // == IA ==
        ID3D12GraphicsCommandList6* cmdList = renderPipeline->getCommandList();
        cmdList->IASetVertexBuffers(0, 1, m->getVBV());
        cmdList->IASetIndexBuffer(m->getIBV());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // == PSO ==
        cmdList->SetPipelineState(renderPipeline->getPSO());
        cmdList->SetGraphicsRootSignature(renderPipeline->getRootSignature());

        // == ROOT ==
        ID3D12DescriptorHeap* descriptorHeaps[] = { renderPipeline->getDescriptorHeap()->getAddress() };
        cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        DirectX::XMMATRIX viewMat = camera->getViewMat();
        DirectX::XMMATRIX projMat = camera->getProjMat();
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewMat, 0);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &projMat, 16);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, m->getModelMatrix(), 32);

        Texture& diffuseTex = *m->getDiffuseTexture();
        cmdList->SetGraphicsRootDescriptorTable(1, diffuseTex.getTextureGpuDescriptorHandle());

        cmdList->DrawIndexedInstanced(m->getNumTriangles() * 3, 1, 0, 0, 0);
    }
}

size_t PbrDrawable::getTriangleCount() {
    return triangleCount;
}

void PbrDrawable::releaseResources() {
    for (Mesh* m : meshes) {
        m->releaseResources();
    }
    renderPipeline->releaseResources();
}