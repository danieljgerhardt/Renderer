#include "EnvironmentMapDrawable.h"

#include "D3D/ResourceManager.h"

#include "Scene/Util/CubeMapGeometry.h"

EnvironmentMapDrawable::EnvironmentMapDrawable(DXContext* context, RenderPipeline* pipeline, Texture* envCubeMap) 
    : context(context), renderPipeline(pipeline), envCubeMap(envCubeMap) {
    construct();
}

void EnvironmentMapDrawable::construct() {
	ResourceManager& rm = ResourceManager::get();

    createCubeGeometry(context, renderPipeline, cubeIbv, cubeVbv);

    triangleCount = 12;
}

void EnvironmentMapDrawable::draw(Camera* camera, D3D12_VIEWPORT& vp) {
    ID3D12GraphicsCommandList6* cmdList = renderPipeline->getCommandList();
    Window::get().setCmdListRenderTarget(cmdList);
    Window::get().setViewport(vp, cmdList);
    
    // == IA ==
    cmdList->IASetVertexBuffers(0, 1, &cubeVbv);
    cmdList->IASetIndexBuffer(&cubeIbv);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // == PSO ==
    cmdList->SetPipelineState(renderPipeline->getPSO());
    cmdList->SetGraphicsRootSignature(renderPipeline->getRootSignature());

    // == ROOT ==
    ID3D12DescriptorHeap* descriptorHeaps[] = { renderPipeline->getDescriptorHeap()->getAddress() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	XMMATRIX viewProj = camera->getViewProjOrientOnly();
    cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewProj, 0);

    cmdList->SetGraphicsRootDescriptorTable(1, envCubeMap->getSrvGpuDescriptorHandle());

    cmdList->DrawIndexedInstanced(UINT(triangleCount) * 3, 1, 0, 0, 0);

    context->executeCommandList(renderPipeline->getCommandListID());
    context->resetCommandList(renderPipeline->getCommandListID());
}

size_t EnvironmentMapDrawable::getTriangleCount() {
    return triangleCount;
}

void EnvironmentMapDrawable::releaseResources() {
    renderPipeline->releaseResources();
}