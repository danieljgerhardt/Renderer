#include "PtScene.h"

#include "D3D/ResourceManager.h"

PtScene::PtScene(Camera* camera, DXContext* context, D3D12_VIEWPORT vp) : Scene(camera, context) {
	ResourceManager& rm = ResourceManager::get();
	ResourceHandle renderHeapHandle = rm.createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 20,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	DescriptorHeap* renderHeap = rm.getDescriptorHeap(renderHeapHandle);

	rayPipeline = std::make_unique<RayPipeline>(*context, RAY_ID, renderHeap);
	primaryPipeline = rayPipeline.get();

	std::vector<float> cubeVertices{
		-1, -1, -1,
		 1, -1, -1,
		-1,  1, -1,
		 1,  1, -1,
		-1, -1,  1,
		 1, -1,  1,
		-1,  1,  1,
		 1,  1,  1,
	};
	ResourceHandle cubeVbHandle = ResourceManager::get().createVertexBuffer(cubeVertices.data(), (UINT)(cubeVertices.size() * sizeof(float)), sizeof(float) * 3);
	VertexBuffer* cubeVb = ResourceManager::get().getVertexBuffer(cubeVbHandle);
	cubeVb->passVertexDataToGPU(*context, rayPipeline->getCommandList());

	std::vector<float> quadVertices{ 
		-1, 0, -1,
		-1, 0,  1,
		 1, 0,  1,
		 1, 0, -1
	};
	ResourceHandle quadVbHandle = ResourceManager::get().createVertexBuffer(quadVertices.data(), (UINT)(quadVertices.size() * sizeof(float)), sizeof(float) * 3);
	VertexBuffer* quadVb = ResourceManager::get().getVertexBuffer(quadVbHandle);
	quadVb->passVertexDataToGPU(*context, rayPipeline->getCommandList());

	std::vector<UINT> cubeIndices{
		4, 6, 0, 2, 0, 6, 0, 1, 4, 5, 4, 1,
		0, 2, 1, 3, 1, 2, 1, 3, 5, 7, 5, 3,
		2, 6, 3, 7, 3, 6, 4, 5, 6, 7, 6, 5
	};
	ResourceHandle cubeIbHandle = ResourceManager::get().createIndexBuffer(cubeIndices, (UINT)(cubeIndices.size() * sizeof(UINT)));
	IndexBuffer* cubeIb = ResourceManager::get().getIndexBuffer(cubeIbHandle);
	cubeIb->passIndexDataToGPU(*context, rayPipeline->getCommandList());

	std::vector<UINT> quadIndices{
		0, 1, 2,
		0, 2, 3
	};
	ResourceHandle quadIbHandle = ResourceManager::get().createIndexBuffer(quadIndices, (UINT)(quadIndices.size() * sizeof(UINT)));
	IndexBuffer* quadIb = ResourceManager::get().getIndexBuffer(quadIbHandle);
	quadIb->passIndexDataToGPU(*context, rayPipeline->getCommandList());

	std::vector<std::string> inputStrings;
	inputStrings.push_back("objs\\Avocado\\Avocado.gltf");
	XMFLOAT4X4 identity;
	XMStoreFloat4x4(&identity, XMMatrixScaling(10.f, 10.f, 10.f));

	GltfData gltfData = Loader::getDataFromGltf((std::filesystem::current_path() / inputStrings.front()).string(), context, rayPipeline->getCommandList(), rayPipeline.get(), identity);
	Mesh* loadedMesh = gltfData.meshes.front();

	quadVb->transitionState(rayPipeline->getCommandList(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cubeVb->transitionState(rayPipeline->getCommandList(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	quadIb->transitionState(rayPipeline->getCommandList(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cubeIb->transitionState(rayPipeline->getCommandList(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//accel structure
	ID3D12Resource* quadBlas;
	ID3D12Resource* cubeBlas;
	ID3D12Resource* meshBlas;
	quadBlas = makeBlas(rayPipeline.get(), quadVb, (UINT)(quadVertices.size() * 3), quadIb, (UINT)quadIndices.size());
	cubeBlas = makeBlas(rayPipeline.get(), cubeVb, (UINT)(cubeVertices.size() * 3), cubeIb, (UINT)cubeIndices.size());
	meshBlas = makeBlas(rayPipeline.get(), loadedMesh);

	blases.push_back(quadBlas);
	blases.push_back(cubeBlas);
	blases.push_back(meshBlas);

	numInstances = 4;

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

	for (UINT i = 0; i < numInstances - 1; i++) {
		instanceData[i] = {
			.InstanceID = 0,
			.InstanceMask = 1,
			.AccelerationStructure = (i ? quadBlas : cubeBlas)->GetGPUVirtualAddress(),
		};
	}

	instanceData[3] = {
		.InstanceID = 0,
		.InstanceMask = 1,
		.AccelerationStructure = meshBlas->GetGPUVirtualAddress(),
	};

	initTopLevel();

	updateScene();

	//create pt target
	TextureData texData;
	texData.width = (UINT)vp.Width;
	texData.height = (UINT)vp.Height;
	texData.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texData.type = TextureType::PT_TARGET;

	ResourceHandle rtHandle = rm.createTexture(rayPipeline.get(), texData);

	renderTarget = rm.getTexture(rtHandle);
	renderTarget->makeUav(context, rayPipeline.get());
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
		
	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	context->executeCommandList(rayPipeline->getCommandListID());
	context->resetCommandList(rayPipeline->getCommandListID());
	scratch->Release();
	return accelStruct;
}

ID3D12Resource* PtScene::makeBlas(RayPipeline* rayPipeline, VertexBuffer* vertexBuffer, UINT vertexFloats, IndexBuffer* indexBuffer, UINT indices) {
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {
		.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
		.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
		.Triangles = {
			.Transform3x4 = 0,
			.IndexFormat = DXGI_FORMAT_R32_UINT,
			.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
			.IndexCount = indices,
			.VertexCount = vertexFloats / 3,
			.IndexBuffer = indexBuffer->getIndexBuffer()->GetGPUVirtualAddress(),
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

ID3D12Resource* PtScene::makeBlas(RayPipeline* rayPipeline, Mesh* mesh)
{
	//get transform3x4 from mesh's model matrix
	XMFLOAT4X4 modelMat = *mesh->getModelMatrix();
	XMFLOAT3X4 modelMat3x4 = XMFLOAT3X4(
		modelMat._11, modelMat._12, modelMat._13, modelMat._14,
		modelMat._21, modelMat._22, modelMat._23, modelMat._24,
		modelMat._31, modelMat._32, modelMat._33, modelMat._34);

	//create buffer for mat
	ResourceManager& rm = ResourceManager::get();
	StructuredBuffer* modelMatBuffer = rm.getStructuredBuffer(rm.createStructuredBuffer(rayPipeline, &modelMat3x4, 12, sizeof(float)));
	modelMatBuffer->passDataToGpu(*context, rayPipeline->getCommandList(), rayPipeline->getCommandListID());

	D3D12_GPU_VIRTUAL_ADDRESS modelMatAddr = modelMatBuffer->getBuffer()->GetGPUVirtualAddress();

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {
		.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
		.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
		.Triangles = {
			.Transform3x4 = modelMatAddr,
			.IndexFormat = DXGI_FORMAT_R32_UINT,
			.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
			.IndexCount = mesh->getIndexCount(),
			.VertexCount = mesh->getVertexCount(),
			.IndexBuffer = mesh->getIndexBuffer()->getIndexBuffer()->GetGPUVirtualAddress(),
			.VertexBuffer = {.StartAddress = mesh->getVertexBuffer()->getVertexBuffer()->GetGPUVirtualAddress(),
							  .StrideInBytes = mesh->getVertexDataStride() }}
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

	if (!tlas || !tlasUpdateScratch) {
		throw std::runtime_error("Failed to create TLAS or TLAS update scratch buffer.");
	}
}

//per frame updates to instance transforms
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

	XMMATRIX scaleUp = XMMatrixScaling(50.f, 50.f, 50.f);
	set(3, scaleUp);
}

//per frame updates to TLAS
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

	cmdList->SetPipelineState1(rayPipeline->getStateObject());
	cmdList->SetComputeRootSignature(rayPipeline->getRootSignature());

	ID3D12DescriptorHeap* descriptorHeaps[] = { rayPipeline->getDescriptorHeap()->getAddress() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE uavTable = renderTarget->getUavGpuDescriptorHandle();

	XMVECTOR camPos = camera->getPositionVector();
	XMVECTOR camForward = camera->getForwardVector();
	XMVECTOR camRight = camera->getRightVector();
	XMVECTOR camUp = camera->getUpVector();

	//set cam pos y to fovy
	XMVectorSetW(camPos, camera->getFovY());

	cmdList->SetComputeRoot32BitConstants(0, 4, &camPos, 0);
	cmdList->SetComputeRoot32BitConstants(0, 4, &camForward, 4);
	cmdList->SetComputeRoot32BitConstants(0, 4, &camRight, 8);
	cmdList->SetComputeRoot32BitConstants(0, 4, &camUp, 12);
	cmdList->SetComputeRootDescriptorTable(1, uavTable);
	cmdList->SetComputeRootShaderResourceView(2, tlas->GetGPUVirtualAddress());

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
		.Width = (UINT)vp.Width,
		.Height = (UINT)vp.Height,
		.Depth = 1 };
	cmdList->DispatchRays(&dispatchDesc);

	//transition back buffer to copy dest
	ID3D12Resource1* backBuffer = Window::get().getCurrentBackBuffer();

	D3D12_RESOURCE_BARRIER barrier = { .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
									.Transition = {
										.pResource = backBuffer,
										.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
										.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
										.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST } };
	cmdList->ResourceBarrier(1, &barrier);

	//transition pt target to copy source
	barrier = { .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
									.Transition = {
										.pResource = renderTarget->getTextureResource(),
										.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
										.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
										.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE } };
	cmdList->ResourceBarrier(1, &barrier);

	//copy to backbuffer
	cmdList->CopyResource(Window::get().getCurrentBackBuffer(), renderTarget->getTextureResource());

	//transition back to render target
	barrier = { .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
									.Transition = {
										.pResource = backBuffer,
										.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
										.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
										.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET } };
	cmdList->ResourceBarrier(1, &barrier);

	//transition pt target back to uav
	barrier = { .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
									.Transition = {
										.pResource = renderTarget->getTextureResource(),
										.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
										.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE,
										.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS } };
	cmdList->ResourceBarrier(1, &barrier);

	context->executeCommandList(rayPipeline->getCommandListID());
	context->resetCommandList(rayPipeline->getCommandListID());
}

void PtScene::releaseResources() {
	if (instances) {
		instances->Unmap(0, nullptr);
		instances->Release();
		instances = nullptr;
	}
	if (tlas) {
		tlas->Release();
		tlas = nullptr;
	}
	if (tlasUpdateScratch) {
		tlasUpdateScratch->Release();
		tlasUpdateScratch = nullptr;
	}
	rayPipeline->releaseResources();
	rayPipeline.reset();
	if (renderTarget) {
		renderTarget->releaseResources();
		renderTarget = nullptr;
	}
	for (ID3D12Resource* blas : blases) {
		if (blas) {
			blas->Release();
		}
	}
}

size_t PtScene::getTriangleCount() {
	return 12 + 8;
}
