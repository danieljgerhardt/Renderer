#include "RayPipeline.h"

RayPipeline::RayPipeline(DXContext& context, CommandListID id, DescriptorHeap* dh)
	: Pipeline(context, id), shaderLib("RayGeneration.hlsl", "Miss.hlsl", "ClosestHit.hlsl")
{
	createRootSignature(context, {});
	descriptorHeap = dh;
	createPipelineState(context.getDevice());
	initShaderTables();
}

void RayPipeline::releaseResources() {
    shaderLib.releaseResources();
	if (shaderIds) {
		shaderIds->Release();
		shaderIds = nullptr;
	}
	if (pso) {
		pso->Release();
		pso = nullptr;
	}
}

void RayPipeline::createRootSignature(DXContext& context, std::vector<Shader*> shaders) {
    generateRootSignature(context);
}

void RayPipeline::generateRootSignature(DXContext& context) {
    RootSignatureBuilder builder;

	const ShaderReflectionData& reflectionData = shaderLib.getReflectionData();

    const std::vector<ShaderResourceBinding>& resources = reflectionData.resources;
    const std::vector<ConstantBufferReflection>& constantBuffers = reflectionData.constantBuffers;

    numDescriptors += static_cast<UINT>(resources.size());
    numDescriptors += static_cast<UINT>(constantBuffers.size());

    //cbvs, srvs, uavs, samplers, accel structs
    std::array<std::vector<ShaderResourceBinding>, 5> resourceBindings;

    for (size_t i = 0; i < resources.size(); i++) {
        const ShaderResourceBinding& res = resources[i];

        switch (res.type) {
        case ShaderResourceType::RootConstant:
        case ShaderResourceType::ConstantBuffer:
            resourceBindings[0].push_back(res);
            break;
        case ShaderResourceType::Texture:
        case ShaderResourceType::StructuredBuffer:
            resourceBindings[1].push_back(res);
            break;
        case ShaderResourceType::RWTexture:
        case ShaderResourceType::RWStructuredBuffer:
            resourceBindings[2].push_back(res);
            break;
        case ShaderResourceType::Sampler:
            resourceBindings[3].push_back(res);
            break;
		case ShaderResourceType::AccelerationStructure:
			resourceBindings[4].push_back(res);
			break;
        }
    }

    UINT rootParamIdx = 0;
    for (const ShaderResourceBinding& cb : resourceBindings[0]) {
        if (cb.type == ShaderResourceType::RootConstant) {
            //assume size is in bytes, convert to 32-bit values
            UINT num32BitValues = cb.size / 4;
            builder.addRootConstant(cb.bindPoint, cb.space, num32BitValues, cb.visibility);
            rootParamIdx++;
            continue;
        }
        builder.addConstantBufferView(cb.bindPoint, cb.space, D3D12_SHADER_VISIBILITY_ALL);
        rootParamIdx++;
    }

    for (const ShaderResourceBinding& srv : resourceBindings[1]) {
        builder.addShaderResourceView(srv.bindPoint, srv.space, srv.visibility);
        rootParamIdx++;
    }

    for (const ShaderResourceBinding& uav : resourceBindings[2]) {
        builder.addUnorderedAccessView(uav.bindPoint, uav.space, uav.visibility);
        rootParamIdx++;
    }

    for (const ShaderResourceBinding& sampler : resourceBindings[3]) {
        builder.addStaticSampler(sampler.bindPoint, sampler.space);
    }

    for (const ShaderResourceBinding& as : resourceBindings[4]) {
        builder.addAccelerationStructure(as.bindPoint, as.space, as.visibility);
    }

    builder.build(context.getDevice().Get(), rootSignature);
}

void RayPipeline::createPSOD() {
    //todo - unneeded?
}

void RayPipeline::createPipelineState(ComPointer<ID3D12Device6>& device) {
    D3D12_DXIL_LIBRARY_DESC lib = {
        .DXILLibrary = {.pShaderBytecode = shaderLib.getBuffer(),
                        .BytecodeLength = shaderLib.getSize()}
    };

    D3D12_HIT_GROUP_DESC hitGroup = { .HitGroupExport = L"HitGroup",
                                     .Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
                                     .ClosestHitShaderImport = L"ClosestHit" };

    D3D12_RAYTRACING_SHADER_CONFIG shaderCfg = {
        .MaxPayloadSizeInBytes = 20,
        .MaxAttributeSizeInBytes = 8,
    };

    D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { rootSignature };

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = { .MaxTraceRecursionDepth = 2 };

    D3D12_STATE_SUBOBJECT subobjects[] = {
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &lib},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &hitGroup},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, .pDesc = &shaderCfg},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &globalSig},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, .pDesc = &pipelineCfg} };
    D3D12_STATE_OBJECT_DESC desc = { 
        .Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
        .NumSubobjects = std::size(subobjects),
        .pSubobjects = subobjects };
    HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		throw std::runtime_error("Could not create raytracing pipeline state object");
	}
}

void RayPipeline::initShaderTables() {
    DXGI_SAMPLE_DESC NO_AA = { .Count = 1, .Quality = 0 };
    D3D12_RESOURCE_DESC idDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = shaderCount * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .SampleDesc = NO_AA,
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR };
    D3D12_HEAP_PROPERTIES UPLOAD_HEAP = { .Type = D3D12_HEAP_TYPE_UPLOAD };

    context.getDevice()->CreateCommittedResource(&UPLOAD_HEAP, D3D12_HEAP_FLAG_NONE, &idDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&shaderIds));

    ID3D12StateObjectProperties* props;
    pso->QueryInterface(&props);

    void* data;
    auto writeId = [&](const wchar_t* name) {
        void* id = props->GetShaderIdentifier(name);
        memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        data = static_cast<char*>(data) +
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        };

    shaderIds->Map(0, nullptr, &data);
    writeId(L"RayGeneration");
    writeId(L"Miss");
    writeId(L"HitGroup");
    shaderIds->Unmap(0, nullptr);

    props->Release();
}
