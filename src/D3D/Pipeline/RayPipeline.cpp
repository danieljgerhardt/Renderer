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
    //todo
}

void RayPipeline::createRootSignature(DXContext& context, std::vector<Shader*> shaders) {
    D3D12_DESCRIPTOR_RANGE uavRange = {
       .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
       .NumDescriptors = 1,
    };
	D3D12_DESCRIPTOR_RANGE cbvRange = {
	   .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
	   .NumDescriptors = 1,
	};
    D3D12_ROOT_PARAMETER params[] = {
        {.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
         .DescriptorTable = {.NumDescriptorRanges = 1,
                             .pDescriptorRanges = &uavRange}},
                             {.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
         .Descriptor = {.ShaderRegister = 0, .RegisterSpace = 0}},
		{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
		 .DescriptorTable = {.NumDescriptorRanges = 1,
							 .pDescriptorRanges = &cbvRange}
        } };

    D3D12_ROOT_SIGNATURE_DESC desc = { .NumParameters = std::size(params),
                                      .pParameters = params };

    ID3DBlob* blob;
    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob,
        nullptr);
    context.getDevice()->CreateRootSignature(0, blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));
    blob->Release();
}

void RayPipeline::generateRootSignature(DXContext& context, Shader& vertexShader, Shader& pixelShader) {
    //todo
}

void RayPipeline::createPSOD() {
    //todo - unneeded?
}

void RayPipeline::createPipelineState(ComPointer<ID3D12Device6>& device) {
    /*D3D12_EXPORT_DESC exports[] = {
        { .Name = L"RayGeneration", .ExportToRename = nullptr, .Flags = D3D12_EXPORT_FLAG_NONE },
		{ .Name = L"Miss", .ExportToRename = nullptr, .Flags = D3D12_EXPORT_FLAG_NONE },
        { .Name = L"ClosestHit", .ExportToRename = nullptr, .Flags = D3D12_EXPORT_FLAG_NONE }
    };*/

    D3D12_DXIL_LIBRARY_DESC lib = {
        .DXILLibrary = {.pShaderBytecode = shaderLib.getBuffer(),
                        .BytecodeLength = shaderLib.getSize()},
        //.NumExports = _countof(exports),
        //.pExports = exports
    };

    D3D12_HIT_GROUP_DESC hitGroup = { .HitGroupExport = L"HitGroup",
                                     .Type = D3D12_HIT_GROUP_TYPE_TRIANGLES,
                                     .ClosestHitShaderImport = L"ClosestHit" };

    D3D12_RAYTRACING_SHADER_CONFIG shaderCfg = {
        .MaxPayloadSizeInBytes = 20,
        .MaxAttributeSizeInBytes = 8,
    };

    D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { rootSignature };

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = { .MaxTraceRecursionDepth = 3 };

    D3D12_STATE_SUBOBJECT subobjects[] = {
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &lib},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &hitGroup},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, .pDesc = &shaderCfg},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &globalSig},
        {.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, .pDesc = &pipelineCfg} };
    D3D12_STATE_OBJECT_DESC desc = { .Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
                                    .NumSubobjects = std::size(subobjects),
                                    .pSubobjects = subobjects };
    device->CreateStateObject(&desc, IID_PPV_ARGS(&pso));
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
    writeId(L"HitGroup");
    writeId(L"Miss");
    shaderIds->Unmap(0, nullptr);

    props->Release();
}
