#include "MeshPipeline.h"

#include "D3D/ResourceManager.h"

MeshPipeline::MeshPipeline(std::string meshShaderName, std::string fragShaderName, DXContext& context,
    CommandListID cmdID, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: Pipeline(context, cmdID), meshShader(meshShaderName, ShaderType::MeshShader), fragShader(fragShaderName, ShaderType::PixelShader)
{
	createRootSignature(context, { &meshShader, &fragShader });

    createPSOD();
    createPipelineState(context.getDevice());
}

void MeshPipeline::createPSOD() {
    psod.pRootSignature = rootSignature;
    psod.MS.BytecodeLength = meshShader.getSize();
    psod.MS.pShaderBytecode = meshShader.getBuffer();
    psod.PS.BytecodeLength = fragShader.getSize();
    psod.PS.pShaderBytecode = fragShader.getBuffer();

    // Primitive topology type for mesh pipelines
    psod.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Render target formats
    psod.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psod.NumRenderTargets = 1;

    // Depth-stencil format
    psod.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    // Other states
    psod.SampleDesc.Count = 1;
    psod.SampleMask = UINT_MAX;
    psod.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psod.RasterizerState.FrontCounterClockwise = TRUE;
    psod.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psod.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psod.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
}

void MeshPipeline::createPipelineState(ComPointer<ID3D12Device6>& device) {
    CD3DX12_PIPELINE_MESH_STATE_STREAM psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psod);
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { sizeof(psoStream), &psoStream };
    HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create compute pipeline state");
    }
}

void MeshPipeline::createRootSignature(DXContext& context, std::vector<Shader*> shaders) {
	if (shaders.size() != 2) {
		throw std::runtime_error("MeshPipeline::createRootSignature requires exactly 2 shaders (mesh and pixel)");
	}
	generateRootSignature(context, *shaders[0], *shaders[1]);
}

void MeshPipeline::generateRootSignature(DXContext& context, Shader& meshShader, Shader& pixelShader) {
    ComPointer<ID3D12Device6> device = context.getDevice();

    RootSignatureBuilder builder;

    std::vector<ShaderResourceBinding> resources;
    std::vector<ConstantBufferReflection> constantBuffers;
    Shader::mergeShaderReflections(meshShader.getReflectionData(), pixelShader.getReflectionData(), resources, constantBuffers);

    numDescriptors += static_cast<UINT>(resources.size());
    numDescriptors += static_cast<UINT>(constantBuffers.size());

    //cbvs, srvs, uavs, samplers
    std::array<std::vector<ShaderResourceBinding>, 4> resourceBindings;

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
        }
    }

    UINT rootParamIdx = 0;
    for (const auto& cb : resourceBindings[0]) {
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

    for (const auto& srv : resourceBindings[1]) {
        builder.addShaderResourceView(srv.bindPoint, srv.space, srv.visibility);
        rootParamIdx++;
    }

    for (const auto& uav : resourceBindings[2]) {
        builder.addUnorderedAccessView(uav.bindPoint, uav.space, uav.visibility);
        rootParamIdx++;
    }

    for (const auto& sampler : resourceBindings[3]) {
        builder.addStaticSampler(sampler.bindPoint, sampler.space);
    }

    builder.build(device.Get(), rootSignature);
}
