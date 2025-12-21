#include "ComputePipeline.h"

#include "D3D/ResourceManager.h"

ComputePipeline::ComputePipeline(const std::string shaderFilePath, DXContext& context,
	CommandListID cmdID, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: Pipeline(context, cmdID),
	computeShader(shaderFilePath, ShaderType::ComputeShader)
{
	createRootSignature(context, { &computeShader });

    ResourceManager& manager = ResourceManager::get(&context);
    descriptorHeap = manager.getDescriptorHeap(manager.createDescriptorHeap(type, numDescriptors, flags));

	createPSOD();
	createPipelineState(context.getDevice());
}

void ComputePipeline::createPSOD() {
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader.getBuffer(), computeShader.getSize());
}

void ComputePipeline::createPipelineState(ComPointer<ID3D12Device6>& device) {
	HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to create compute pipeline state");
	}
}

void ComputePipeline::createRootSignature(DXContext& context, std::vector<Shader*> shaders) {
	if (shaders.size() != 1) {
		throw std::runtime_error("ComputePipeline::createRootSignature requires exactly 1 shader (compute)");
	}
	generateRootSignature(context, *shaders[0]);
}

void ComputePipeline::generateRootSignature(DXContext& context, Shader& computeShader) {
    ComPointer<ID3D12Device6> device = context.getDevice();

    RootSignatureBuilder builder;

	ShaderReflectionData& reflectionData = computeShader.getReflectionData();

    std::vector<ShaderResourceBinding>& resources = reflectionData.resources;
    std::vector<ConstantBufferReflection>& constantBuffers = reflectionData.constantBuffers;

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
