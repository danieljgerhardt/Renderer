#include "PbrDrawable.h"

PbrDrawable::PbrDrawable(DXContext* context, RenderPipeline* pipeline) : context(context), renderPipeline(pipeline) {
    construct();
}

void PbrDrawable::construct() {
    std::vector<std::string> inputStrings;
    inputStrings.push_back("objs\\Avocado\\Avocado.gltf");
	inputStrings.push_back("objs\\Helmet\\DamagedHelmet.gltf");

    XMFLOAT4X4 avocadoModelMatrix;
    XMStoreFloat4x4(&avocadoModelMatrix, XMMatrixMultiply(
        XMMatrixScaling(1000.f, 1000.f, 1000.f),
        XMMatrixTranslation(-0.f, 0.f, 50.f)
    ));
    modelMatrices.push_back(avocadoModelMatrix);

	XMFLOAT4X4 helmetModelMatrix;
	XMStoreFloat4x4(&helmetModelMatrix, XMMatrixMultiply(
		XMMatrixScaling(10.f, 10.f, 10.f),
		XMMatrixTranslation(30.f, 20.f, 20.f)
	));
	XMStoreFloat4x4(&helmetModelMatrix, XMMatrixMultiply(XMMatrixRotationRollPitchYaw(90.f, 135.f, 0.f), XMLoadFloat4x4(&helmetModelMatrix)));
	modelMatrices.push_back(helmetModelMatrix);

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

void PbrDrawable::draw(Camera* camera, D3D12_VIEWPORT& vp) {
    ID3D12GraphicsCommandList6* cmdList = renderPipeline->getCommandList();
    Window::get().setCmdListRenderTarget(cmdList);
    Window::get().setViewport(vp, cmdList);

    std::array<UINT, 2> brdfLutUsage = { 0, 1 };
    UINT brdfLutUsageIdx = 0;

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
		DirectX::XMVECTOR pos = camera->getPositionVector();
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewMat, 0);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, &projMat, 16);
        cmdList->SetGraphicsRoot32BitConstants(0, 16, m->getModelMatrix(), 32);
		cmdList->SetGraphicsRoot32BitConstants(0, 4, &pos, 48);
		cmdList->SetGraphicsRoot32BitConstants(0, 1, &brdfLutUsage[brdfLutUsageIdx], 52);
        brdfLutUsageIdx++;

        cmdList->SetGraphicsRootDescriptorTable(1, m->getTexture(TextureType::DIFFUSE)->getSrvGpuDescriptorHandle());
        cmdList->SetGraphicsRootDescriptorTable(2, m->getTexture(TextureType::METALLIC_ROUGHNESS)->getSrvGpuDescriptorHandle());

		cmdList->SetGraphicsRootDescriptorTable(3, diffuseConvolution->getSrvGpuDescriptorHandle());
		cmdList->SetGraphicsRootDescriptorTable(4, glossyConvolution->getSrvGpuDescriptorHandle());
		cmdList->SetGraphicsRootDescriptorTable(5, brdfLut->getSrvGpuDescriptorHandle());

        cmdList->DrawIndexedInstanced(m->getNumTriangles() * 3, 1, 0, 0, 0);
    }
    context->executeCommandList(renderPipeline->getCommandListID());
    context->resetCommandList(renderPipeline->getCommandListID());
}

void PbrDrawable::setIblTextures(Texture* diffuseConvolution, Texture* glossyConvolution, Texture* brdfLut) {
	this->diffuseConvolution = diffuseConvolution;
	this->glossyConvolution = glossyConvolution;
	this->brdfLut = brdfLut;
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