#include "ObjectScene.h"

ObjectScene::ObjectScene(DXContext* context, RenderPipeline* pipeline)
	: Drawable(context, pipeline)
{
    constructSceneSolid();
}

void ObjectScene::constructSceneSolid() {
    //TODO - this code should call loader for objects which should reutnr meshes that can be used here
    std::vector<std::string> inputStrings;
    //inputStrings.push_back("objs\\cube.obj");
    inputStrings.push_back("objs\\Avocado\\Avocado.gltf");

    XMFLOAT4X4 groundModelMatrix;
	XMStoreFloat4x4(&groundModelMatrix, XMMatrixMultiply(
		XMMatrixScaling(1000.f, 1000.f, 1000.f),
		XMMatrixTranslation(0.f, 0.f, 0.f)
	));
    modelMatrices.push_back(groundModelMatrix);

    // vector for colors of grid lines
    std::vector<XMFLOAT3> colors = { XMFLOAT3(1.f, 0.f, 0.f) };

    //push ground as solid
    auto string = inputStrings.front();
    auto m = modelMatrices.front();
    //Mesh newMesh = Mesh((std::filesystem::current_path() / string).string(), context, renderPipeline->getCommandList(), renderPipeline, m, colors.front());
	Mesh newMesh = Loader::createMeshFromGltf((std::filesystem::current_path() / string).string(), context, renderPipeline->getCommandList(), renderPipeline, m, colors.front())[0];
    meshes.push_back(newMesh);
    sceneSize += newMesh.getNumTriangles();
}

void ObjectScene::draw(Camera* camera) {
    for (Mesh m : meshes) {
        // == IA ==
        auto cmdList = renderPipeline->getCommandList();
        cmdList->IASetVertexBuffers(0, 1, m.getVBV());
        cmdList->IASetIndexBuffer(m.getIBV());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // == PSO ==
        cmdList->SetPipelineState(renderPipeline->getPSO());
        cmdList->SetGraphicsRootSignature(renderPipeline->getRootSignature());
        
        // == ROOT ==
        ID3D12DescriptorHeap* descriptorHeaps[] = { renderPipeline->getDescriptorHeap()->GetAddress() };
        if (instanced) {
            cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        }

        auto viewMat = camera->getViewMat();
        auto projMat = camera->getProjMat();
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewMat, 0);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &projMat, 16);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, m.getModelMatrix(), 32);
        cmdList->SetGraphicsRoot32BitConstants(0, 3, m.getColor(), 48);
        cmdList->DrawIndexedInstanced(m.getNumTriangles() * 3, 1, 0, 0, 0);
    }
}

size_t ObjectScene::getSceneSize() {
    return sceneSize;
}

void ObjectScene::releaseResources() {
	for (Mesh m : meshes) {
		m.releaseResources();
	}
    renderPipeline->releaseResources();
}