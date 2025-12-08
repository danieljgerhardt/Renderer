#include "ObjectScene.h"

ObjectScene::ObjectScene(DXContext* context, RenderPipeline* pipeline)
	: Drawable(context, pipeline)
{
    constructSceneSolid();
}

void ObjectScene::constructSceneSolid() {
    std::vector<std::string> inputStrings;
    inputStrings.push_back("objs\\Avocado\\Avocado.gltf");

    XMFLOAT4X4 avocadoModelMatrix;
	XMStoreFloat4x4(&avocadoModelMatrix, XMMatrixMultiply(
		XMMatrixScaling(1000.f, 1000.f, 1000.f),
		XMMatrixTranslation(0.f, 0.f, 0.f)
	));
    modelMatrices.push_back(avocadoModelMatrix);

    auto string = inputStrings.front();
    auto m = modelMatrices.front();
   
    gltfData = Loader::createMeshFromGltf((std::filesystem::current_path() / string).string(), context, renderPipeline->getCommandList(), renderPipeline, m);
    Mesh& newMesh = gltfData.meshes[0];
    newMesh.assignTextures(gltfData.textures[0], gltfData.textures[0], gltfData.textures[0], gltfData.textures[0]);

	newMesh.getDiffuseTexture().makeSrv(context, renderPipeline);
    meshes.push_back(newMesh);
    sceneSize += meshes.back().getNumTriangles();
}

void ObjectScene::draw(Camera* camera) {
    for (Mesh& m : meshes) {
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
        cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        auto viewMat = camera->getViewMat();
        auto projMat = camera->getProjMat();
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewMat, 0);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &projMat, 16);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, m.getModelMatrix(), 32);

		Texture& diffuseTex = m.getDiffuseTexture();
		cmdList->SetGraphicsRootDescriptorTable(2, diffuseTex.getTextureGpuDescriptorHandle());

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