#include "EnvironmentMapDrawable.h"

EnvironmentMapDrawable::EnvironmentMapDrawable(DXContext* context, RenderPipeline* pipeline, Texture* envCubeMap) 
    : context(context), renderPipeline(pipeline), envCubeMap(envCubeMap) {
    construct();
}

void EnvironmentMapDrawable::construct() {
    XMFLOAT4X4 identityModelMatrix;
    XMStoreFloat4x4(&identityModelMatrix, XMMatrixMultiply(
        XMMatrixScaling(1.f, 1.f, 1.f),
        XMMatrixTranslation(0.f, 0.f, 0.f)
    ));

    std::string inputString = "objs\\cube\\Cube.gltf";
    gltfData = Loader::createMeshFromGltf((std::filesystem::current_path() / inputString).string(), context, renderPipeline->getCommandList(), renderPipeline, identityModelMatrix);
    
    cube = gltfData.meshes[0];

    triangleCount += cube->getNumTriangles();
}

void EnvironmentMapDrawable::draw(Camera* camera, D3D12_VIEWPORT& vp) {
    ID3D12GraphicsCommandList6* cmdList = renderPipeline->getCommandList();
    Window::get().setCmdListRenderTarget(cmdList);
    Window::get().setViewport(vp, cmdList);
    
    // == IA ==
    cmdList->IASetVertexBuffers(0, 1, cube->getVBV());
    cmdList->IASetIndexBuffer(cube->getIBV());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // == PSO ==
    cmdList->SetPipelineState(renderPipeline->getPSO());
    cmdList->SetGraphicsRootSignature(renderPipeline->getRootSignature());

    // == ROOT ==
    ID3D12DescriptorHeap* descriptorHeaps[] = { renderPipeline->getDescriptorHeap()->getAddress() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	XMMATRIX viewProj = camera->getViewProjOrientOnly();
    cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewProj, 0);

    cmdList->SetGraphicsRootDescriptorTable(1, envCubeMap->getTextureGpuDescriptorHandle());

    cmdList->DrawIndexedInstanced(cube->getNumTriangles() * 3, 1, 0, 0, 0);

    context->executeCommandList(renderPipeline->getCommandListID());
    context->resetCommandList(renderPipeline->getCommandListID());
}

size_t EnvironmentMapDrawable::getTriangleCount() {
    return triangleCount;
}

void EnvironmentMapDrawable::releaseResources() {
    cube->releaseResources();
    renderPipeline->releaseResources();
}