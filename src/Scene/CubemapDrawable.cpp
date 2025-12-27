#include "CubemapDrawable.h"

#include "D3D/ResourceManager.h"

CubemapDrawable::CubemapDrawable(DXContext* context, RenderPipeline* pipeline) : context(context), renderPipeline(pipeline) {
	construct();

	//TODO - should only draw once here
}

void CubemapDrawable::draw(Camera* camera, D3D12_VIEWPORT& vp) {
	ID3D12GraphicsCommandList6* cmdList = renderPipeline->getCommandList();
	
	D3D12_VIEWPORT cubemapVp;
	cubemapVp.TopLeftX = 0.f;
	cubemapVp.TopLeftY = 0.f;
	cubemapVp.Width = (float)envCubeMap->getWidth();
	cubemapVp.Height = (float)envCubeMap->getHeight();
	cubemapVp.MinDepth = 0.f;
	cubemapVp.MaxDepth = 1.f;

	Window::get().setViewport(cubemapVp, cmdList);

	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = envCubeMap->getWidth();
	scissorRect.bottom = envCubeMap->getHeight();
	cmdList->RSSetScissorRects(1, &scissorRect);

	CD3DX12_RESOURCE_BARRIER toRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
		envCubeMap->getTextureResource(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	cmdList->ResourceBarrier(1, &toRenderTarget);

	for (int i = 0; i < 6; i++) {
		cmdList->OMSetRenderTargets(
			1,
			&cubeMapSrvHandles[i].first,
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

		cmdList->SetGraphicsRootDescriptorTable(1, envMap->getTextureGpuDescriptorHandle());

		cmdList->DrawIndexedInstanced(12 * 3, 1, 0, 0, 0);
	}

	CD3DX12_RESOURCE_BARRIER toShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
		envCubeMap->getTextureResource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	cmdList->ResourceBarrier(1, &toShaderResource);

	context->executeCommandList(renderPipeline->getCommandListID());
	context->resetCommandList(renderPipeline->getCommandListID());
}

size_t CubemapDrawable::getTriangleCount() {
	return 0;
}

void CubemapDrawable::releaseResources() {
}

void CubemapDrawable::construct() {
	ResourceManager& rm = ResourceManager::get(context);

	ResourceHandle envMapHandle = rm.createTextureFromFile("textures\\environments\\Frozen_Waterfall_Ref.hdr", context, renderPipeline->getCommandList(), renderPipeline, TextureType::DIFFUSE);
	envMap = rm.getTexture(envMapHandle);

	ResourceHandle envCubeMapHandle = rm.createTexture(renderPipeline, 1024, 1024, {}, TextureType::ENV_MAP);
	envCubeMap = rm.getTexture(envCubeMapHandle);

	viewMatrices.resize(6);
	viewMatrices[0] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(1.f, 0.f, 0.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));   // +X
	viewMatrices[1] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(-1.f, 0.f, 0.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));  // -X
	viewMatrices[2] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, 1.f, 0.f, 0.f), XMVectorSet(0.f, 0.f, 1.f, 0.f));    // +Y
	viewMatrices[3] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, -1.f, 0.f, 0.f), XMVectorSet(0.f, 0.f, -1.f, 0.f));  // -Y -- currently broken
	viewMatrices[4] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, 0.f, 1.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));   // +Z
	viewMatrices[5] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, 0.f, -1.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));  // -Z

	//screen spanning quad vert buffer
	std::vector<Vertex> vertData{
		{XMFLOAT3(-1.f, -1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 1.f)},
		{XMFLOAT3(1.f,  -1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 0.f)},
		{XMFLOAT3(1.f,  1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 0.f)},
		{XMFLOAT3(-1.f, 1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 1.f)},
		{XMFLOAT3(-1.f, -1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 1.f)},
		{XMFLOAT3(1.f, -1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 0.f)},
		{XMFLOAT3(1.f,  1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 0.f)},
		{XMFLOAT3(-1.f, 1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 1.f)}
	};
	ResourceHandle vbHandle = rm.createVertexBuffer(renderPipeline, vertData, (UINT)(vertData.size() * sizeof(Vertex)), (UINT)sizeof(Vertex));
	cubeVb = rm.getVertexBuffer(vbHandle);

	std::vector<UINT> indexData{
		1, 0, 3, 1, 3, 2,
		4, 5, 6, 4, 6, 7,
		5, 1, 2, 5, 2, 6,
		7, 6, 2, 7, 2, 3,
		0, 4, 7, 0, 7, 3,
		0, 1, 5, 0, 5, 4
	};
	ResourceHandle ibHandle = rm.createIndexBuffer(renderPipeline, indexData, (UINT)(indexData.size() * sizeof(UINT)));
	cubeIb = rm.getIndexBuffer(ibHandle);

	cubeVbv = cubeVb->passVertexDataToGPU(*context, renderPipeline->getCommandList());
	cubeIbv = cubeIb->passIndexDataToGPU(*context, renderPipeline->getCommandList());

	//Transition both buffers to their usable states
	D3D12_RESOURCE_BARRIER barriers[2] = {};

	// Vertex buffer barrier
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Transition.pResource = cubeVb->getVertexBuffer().Get();
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// Index buffer barrier
	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[1].Transition.pResource = cubeIb->getIndexBuffer().Get();
	barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	renderPipeline->getCommandList()->ResourceBarrier(2, barriers);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = envCubeMap->getResourceDesc().Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;

	rtvDescriptorHeap = rm.getDescriptorHeap(rm.createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 6, D3D12_DESCRIPTOR_HEAP_FLAG_NONE));

	for (int i = 0; i < 6; i++) {
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE rtvGpuHandle;
		rtvDescriptorHeap->allocate(rtvCpuHandle, rtvGpuHandle);
		context->getDevice()->CreateRenderTargetView(envCubeMap->getTextureResource(), &rtvDesc, rtvCpuHandle);
		cubeMapSrvHandles[i] = { rtvCpuHandle, rtvGpuHandle };
	}
}
