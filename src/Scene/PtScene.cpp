#include "PtScene.h"

#include "D3D/ResourceManager.h"

PtScene::PtScene(Camera* camera, DXContext* context) : Scene(camera, context) {
	ResourceManager& rm = ResourceManager::get(context);
	ResourceHandle renderHeapHandle = rm.createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	DescriptorHeap* renderHeap = rm.getDescriptorHeap(renderHeapHandle);

	rayPipeline = std::make_unique<RayPipeline>(*context, RAY_ID, renderHeap);

	std::vector<Vertex> cubeVertices{
		{{-1, -1, -1}, {0, 0, 0}, {0, 0}},
		{{ 1, -1, -1}, {0, 0, 0}, {0, 0}},
		{{-1,  1, -1}, {0, 0, 0}, {0, 0}},
		{{ 1,  1, -1}, {0, 0, 0}, {0, 0}},
		{{-1, -1,  1}, {0, 0, 0}, {0, 0}},
		{{ 1, -1,  1}, {0, 0, 0}, {0, 0}},
		{{-1,  1,  1}, {0, 0, 0}, {0, 0}},
		{{ 1,  1,  1}, {0, 0, 0}, {0, 0}},
	};
	ResourceHandle cubeVbHandle = ResourceManager::get(context).createVertexBuffer(cubeVertices, (UINT)(cubeVertices.size() * sizeof(Vertex)), sizeof(Vertex));
	VertexBuffer* cubeVb = ResourceManager::get(context).getVertexBuffer(cubeVbHandle);
	cubeVb->passVertexDataToGPU(*context, rayPipeline->getCommandList());

	std::vector<Vertex> quadVertices{
		{{-1, 0, -1}, {0, 0, 0}, {0, 0}},
		{{-1, 0, 1}, {0, 0, 0}, {0, 0}},
		{{1, 0, 1}, {0, 0, 0}, {0, 0}},
		{{-1, 0, -1}, {0, 0, 0}, {0, 0}},
		{{1, 0, -1}, {0, 0, 0}, {0, 0}},
		{{1, 0, 1}, {0, 0, 0}, {0, 0}}
	};
	ResourceHandle quadVbHandle = ResourceManager::get(context).createVertexBuffer(quadVertices, (UINT)(quadVertices.size() * sizeof(Vertex)), sizeof(Vertex));
	VertexBuffer* quadVb = ResourceManager::get(context).getVertexBuffer(quadVbHandle);
	quadVb->passVertexDataToGPU(*context, rayPipeline->getCommandList());

	std::vector<UINT> cubeIndices{
		4, 6, 0, 2, 0, 6, 0, 1, 4, 5, 4, 1,
		0, 2, 1, 3, 1, 2, 1, 3, 5, 7, 5, 3,
		2, 6, 3, 7, 3, 6, 4, 5, 6, 7, 6, 5
	};
	ResourceHandle cubeIbHandle = ResourceManager::get(context).createIndexBuffer(cubeIndices, (UINT)(cubeIndices.size() * sizeof(UINT)));
	IndexBuffer* cubeIb = ResourceManager::get(context).getIndexBuffer(cubeIbHandle);
	cubeIb->passIndexDataToGPU(*context, rayPipeline->getCommandList());

	//accel structure
	ID3D12Resource* quadBlas;
	ID3D12Resource* cubeBlas;
	quadBlas = makeBlas(rayPipeline.get(), quadVb, (UINT)(quadVertices.size() * 3), nullptr, 0);
	cubeBlas = makeBlas(rayPipeline.get(), cubeVb, (UINT)(cubeVertices.size() * 3), cubeIb, (UINT)cubeIndices.size());

	constexpr UINT numInstances = 3;
	DXGI_SAMPLE_DESC NO_AA = { .Count = 1, .Quality = 0 };
	D3D12_RESOURCE_DESC instancesDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.SampleDesc = NO_AA,
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR };

	ComPointer<ID3D12Device6>& device = context->getDevice();
	D3D12_HEAP_PROPERTIES UPLOAD_HEAP = { .Type = D3D12_HEAP_TYPE_UPLOAD };
	device->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &instancesDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&instances));
	instances->Map(0, nullptr, reinterpret_cast<void**>(&instanceData));

	for (int i = 0; i < numInstances; i++) {
		instanceData[i] = {
			.InstanceID = 0,
			.InstanceMask = 1,
			.AccelerationStructure = (i ? quadBlas : cubeBlas)->GetGPUVirtualAddress(),
		};
	}

	updateTransforms();
}

ID3D12Resource* PtScene::makeAccelStruct(RayPipeline* rayPipeline, const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs, UINT64* updateScratchSize) {
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	ComPointer<ID3D12Device6>& device = context->getDevice();
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	DXGI_SAMPLE_DESC NO_AA = { .Count = 1, .Quality = 0 };
	D3D12_HEAP_PROPERTIES UPLOAD_HEAP = { .Type = D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES DEFAULT_HEAP = { .Type = D3D12_HEAP_TYPE_DEFAULT };
	D3D12_RESOURCE_DESC BASIC_BUFFER_DESC = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = 0,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.SampleDesc = NO_AA,
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR };

	if (updateScratchSize) {
		*updateScratchSize = prebuildInfo.UpdateScratchDataSizeInBytes;
	}

	D3D12_RESOURCE_DESC scratchDesc = BASIC_BUFFER_DESC;
	scratchDesc.Width = prebuildInfo.ScratchDataSizeInBytes;
	scratchDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ID3D12Resource* scratch;
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &scratchDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&scratch));

	D3D12_RESOURCE_DESC asDesc = BASIC_BUFFER_DESC;
	asDesc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
	asDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ID3D12Resource* accelStruct;
	device->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &asDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&accelStruct));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {
	.DestAccelerationStructureData = accelStruct->GetGPUVirtualAddress(),
	.Inputs = inputs,
	.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress() };

	ID3D12GraphicsCommandList6* cmdList = rayPipeline->getCommandList();
	context->resetCommandList(rayPipeline->getCommandListID());
		
	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	context->executeCommandList(rayPipeline->getCommandListID());
	scratch->Release();
	return accelStruct;
}

ID3D12Resource* PtScene::makeBlas(RayPipeline* rayPipeline, VertexBuffer* vertexBuffer, UINT vertexFloats, IndexBuffer* indexBuffer, UINT indices) {
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {
		.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
		.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
		.Triangles = {
			.Transform3x4 = 0,
			.IndexFormat = indexBuffer ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_UNKNOWN,
			.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
			.IndexCount = indices,
			.VertexCount = vertexFloats / 3,
			.IndexBuffer = indexBuffer ? indexBuffer->getIndexBuffer()->GetGPUVirtualAddress() : 0,
			.VertexBuffer = { .StartAddress = vertexBuffer->getVertexBuffer()->GetGPUVirtualAddress(),
							  .StrideInBytes = vertexBuffer->getVertexDataStride() }}
	};

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
		.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
		.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
		.NumDescs = 1,
		.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
		.pGeometryDescs = &geometryDesc
	};

	return makeAccelStruct(rayPipeline, inputs);
}

ID3D12Resource* PtScene::makeTlas(RayPipeline* rayPipeline, ID3D12Resource* instances, UINT numInstances, UINT64* updateScratchSize) {
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {
		.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
		.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
		.NumDescs = numInstances,
		.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
		.InstanceDescs = instances->GetGPUVirtualAddress() };

	return makeAccelStruct(rayPipeline, inputs, updateScratchSize);
}

void PtScene::initTopLevel() {
	UINT64 updateScratchSize;
	tlas = makeTlas(rayPipeline.get(), instances, numInstances, &updateScratchSize);

	DXGI_SAMPLE_DESC NO_AA = { .Count = 1, .Quality = 0 };
	D3D12_HEAP_PROPERTIES DEFAULT_HEAP = { .Type = D3D12_HEAP_TYPE_DEFAULT };

	D3D12_RESOURCE_DESC tlasBufferDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = std::max(updateScratchSize, 8ULL),
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.SampleDesc = NO_AA,
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS };

	context->getDevice()->CreateCommittedResource(&DEFAULT_HEAP, D3D12_HEAP_FLAG_NONE, &tlasBufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr,
		IID_PPV_ARGS(&tlasUpdateScratch));
}

void PtScene::updateTransforms() {
	using namespace DirectX;
	
	auto set = [this](int idx, XMMATRIX mx) {
		XMFLOAT3X4* ptr = reinterpret_cast<XMFLOAT3X4*>(&instanceData[idx].Transform);
		XMStoreFloat3x4(ptr, mx);
		};

	float time = static_cast<float>(GetTickCount64()) / 1000.f;

	XMMATRIX cubeMat = XMMatrixRotationRollPitchYaw(time * 0.5f, time * 0.33f, time * 0.2f);
	cubeMat *= XMMatrixTranslation(-1.5f, 2.f, 2.f);
	set(0, cubeMat);

	XMMATRIX mirrorMat = XMMatrixRotationX(-1.8f);
	mirrorMat *= XMMatrixRotationY(XMScalarSinEst(time) / 8 + 1);
	mirrorMat *= XMMatrixTranslation(2, 2, 2);
	set(1, mirrorMat);

	XMMATRIX floorMat = XMMatrixScaling(5, 5, 5);
	floorMat *= XMMatrixTranslation(0, 0, 2);
	set(2, floorMat);
}

void PtScene::updateScene() {
	updateTransforms();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {
		.DestAccelerationStructureData = tlas->GetGPUVirtualAddress(),
		.Inputs = {
			.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
			.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE,
			.NumDescs = numInstances,
			.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
			.InstanceDescs = instances->GetGPUVirtualAddress()},
		.SourceAccelerationStructureData = tlas->GetGPUVirtualAddress(),
		.ScratchAccelerationStructureData = tlasUpdateScratch->GetGPUVirtualAddress(),
	};
	rayPipeline->getCommandList()->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);

	D3D12_RESOURCE_BARRIER barrier = { .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
									.UAV = {.pResource = tlas} };
	rayPipeline->getCommandList()->ResourceBarrier(1, &barrier);
}

void PtScene::draw(D3D12_VIEWPORT& vp) {
	updateScene();

	ID3D12GraphicsCommandList6* cmdList = rayPipeline->getCommandList();
	Window::get().setCmdListRenderTarget(cmdList);
	Window::get().setViewport(vp, cmdList);

	cmdList->SetPipelineState(rayPipeline->getPSO());
	cmdList->SetComputeRootSignature(rayPipeline->getRootSignature());

	ID3D12DescriptorHeap* descriptorHeaps[] = { rayPipeline->getDescriptorHeap()->getAddress() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	auto uavTable = descriptorHeaps[0]->GetGPUDescriptorHandleForHeapStart();

	cmdList->SetComputeRootDescriptorTable(0, uavTable);
	cmdList->SetComputeRootShaderResourceView(1, tlas->GetGPUVirtualAddress());

	UINT width = vp.Width;
	UINT height = vp.Height;

	ID3D12Resource* shaderIds = rayPipeline->getShaderIds();

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {
		.RayGenerationShaderRecord = {
			.StartAddress = shaderIds->GetGPUVirtualAddress(),
			.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
		.MissShaderTable = {
			.StartAddress = shaderIds->GetGPUVirtualAddress() +
							D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
			.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
		.HitGroupTable = {
			.StartAddress = shaderIds->GetGPUVirtualAddress() +
							2 * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
			.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES},
		.Width = width,
		.Height = height,
		.Depth = 1 };
	cmdList->DispatchRays(&dispatchDesc);

	context->executeCommandList(rayPipeline->getCommandListID());
	context->resetCommandList(rayPipeline->getCommandListID());
}
