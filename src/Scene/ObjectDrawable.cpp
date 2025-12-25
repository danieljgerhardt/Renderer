#include "ObjectDrawable.h"

ObjectDrawable::ObjectDrawable(DXContext* context, RenderPipeline* pipeline) : context(context), renderPipeline(pipeline) {
    construct();
}

void ObjectDrawable::construct() {
    std::vector<std::string> inputStrings;
    inputStrings.push_back("objs\\Avocado\\Avocado.gltf");
	inputStrings.push_back("objs\\Avocado\\Avocado.gltf");

    XMFLOAT4X4 avocadoModelMatrix;
	XMStoreFloat4x4(&avocadoModelMatrix, XMMatrixMultiply(
		XMMatrixScaling(1000.f, 1000.f, 1000.f),
		XMMatrixTranslation(0.f, 0.f, 30.f)
	));
    modelMatrices.push_back(avocadoModelMatrix);
    XMFLOAT4X4 avocadoModelMatrix2;
    XMStoreFloat4x4(&avocadoModelMatrix2, XMMatrixMultiply(
        XMMatrixScaling(1000.f, 1000.f, 1000.f),
        XMMatrixTranslation(-50.f, 0.f, 30.f)
    ));
    modelMatrices.push_back(avocadoModelMatrix2);

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

void ObjectDrawable::draw(Camera* camera, D3D12_VIEWPORT& vp) {
    ID3D12GraphicsCommandList6* cmdList = renderPipeline->getCommandList();
    Window::get().setCmdListRenderTarget(cmdList);
    Window::get().setViewport(vp, cmdList);
    for (Mesh* m : meshes) {
        // == IA ==
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

		cmdList->SetGraphicsRootDescriptorTable(1, m->getTexture(TextureType::DIFFUSE)->getTextureGpuDescriptorHandle());

        cmdList->DrawIndexedInstanced(m->getNumTriangles() * 3, 1, 0, 0, 0);
    }
    context->executeCommandList(renderPipeline->getCommandListID());
    context->resetCommandList(renderPipeline->getCommandListID());
}

size_t ObjectDrawable::getTriangleCount() {
    return triangleCount;
}

void ObjectDrawable::releaseResources() {
	for (Mesh* m : meshes) {
		m->releaseResources();
	}
    renderPipeline->releaseResources();
}