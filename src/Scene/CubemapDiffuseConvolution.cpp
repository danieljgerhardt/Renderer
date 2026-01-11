#include "CubemapDiffuseConvolution.h"

#include "D3D/ResourceManager.h"

#include "Scene/Util/CubeMapGeometry.h"

CubemapDiffuseConvolution::CubemapDiffuseConvolution(DXContext* context, RenderPipeline* pipeline, Texture* envCubeMap) 
	: context(context), renderPipeline(pipeline), envCubeMap(envCubeMap) {
	construct();

	//TODO - should only draw once here
}

void CubemapDiffuseConvolution::draw(Camera* camera, D3D12_VIEWPORT& vp) {
	ID3D12GraphicsCommandList6* cmdList = renderPipeline->getCommandList();

	D3D12_VIEWPORT cubemapVp;
	cubemapVp.TopLeftX = 0.f;
	cubemapVp.TopLeftY = 0.f;
	cubemapVp.Width = (float)diffuseConvolution->getWidth();
	cubemapVp.Height = (float)diffuseConvolution->getHeight();
	cubemapVp.MinDepth = 0.f;
	cubemapVp.MaxDepth = 1.f;

	Window::get().setViewport(cubemapVp, cmdList);

	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = diffuseConvolution->getWidth();
	scissorRect.bottom = diffuseConvolution->getHeight();

	cmdList->RSSetScissorRects(1, &scissorRect);

	CD3DX12_RESOURCE_BARRIER toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
		diffuseConvolution->getTextureResource(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	cmdList->ResourceBarrier(1, &toRenderTarget);

	for (int i = 0; i < 6; i++) {
		cmdList->OMSetRenderTargets(
			1,
			&diffuseConvolutionSrvHandles[i].first,
			FALSE,
			nullptr
		);

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

		DirectX::XMMATRIX viewMat = viewMatrices[i];
		cmdList->SetGraphicsRoot32BitConstants(0, 16, &viewMat, 0);

		cmdList->SetGraphicsRootDescriptorTable(1, envCubeMap->getSrvGpuDescriptorHandle());

		cmdList->DrawIndexedInstanced(12 * 3, 1, 0, 0, 0);
	}

	CD3DX12_RESOURCE_BARRIER toShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
		diffuseConvolution->getTextureResource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	cmdList->ResourceBarrier(1, &toShaderResource);

	context->executeCommandList(renderPipeline->getCommandListID());
	context->resetCommandList(renderPipeline->getCommandListID());
}

size_t CubemapDiffuseConvolution::getTriangleCount() {
	return 0;
}

void CubemapDiffuseConvolution::releaseResources() {
}

void CubemapDiffuseConvolution::construct() {
	ResourceManager& rm = ResourceManager::get(context);

	TextureData textureData{ .width = 512, .height = 512, .type = TextureType::ENV_MAP };
	ResourceHandle diffuseConvolutionHandle = rm.createTexture(renderPipeline, textureData);
	diffuseConvolution = rm.getTexture(diffuseConvolutionHandle);

	fillCubemapViewMatrices(viewMatrices);

	createCubeGeometry(context, renderPipeline, cubeIbv, cubeVbv);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = diffuseConvolution->getResourceDesc().Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;

	rtvDescriptorHeap = rm.getDescriptorHeap(rm.createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 6, D3D12_DESCRIPTOR_HEAP_FLAG_NONE));

	for (int i = 0; i < 6; i++) {
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE rtvGpuHandle;
		rtvDescriptorHeap->allocate(rtvCpuHandle, rtvGpuHandle);
		context->getDevice()->CreateRenderTargetView(diffuseConvolution->getTextureResource(), &rtvDesc, rtvCpuHandle);
		diffuseConvolutionSrvHandles[i] = { rtvCpuHandle, rtvGpuHandle };
	}
}
