#include "Pipeline.h"

#include "D3D/ResourceManager.h"

Pipeline::Pipeline(DXContext& context, CommandListID cmdID,
	D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: cmdID(cmdID), cmdList(context.createCommandList(cmdID))
{
	//TODO - number of descriptors should be pulled from reflection if possible
	ResourceManager& manager = ResourceManager::get(&context);
	descriptorHeap = manager.getDescriptorHeap(manager.createDescriptorHeap(type, numberOfDescriptors, flags));

  	context.resetCommandList(cmdID);
}

void Pipeline::generateRootSignature(DXContext& context, Shader& vertexShader, Shader& pixelShader)
{
	ComPointer<ID3D12Device6> device = context.getDevice();

	RootSignatureBuilder builder;

	std::vector<ShaderResourceBinding> resources;
	std::vector<ConstantBufferReflection> constantBuffers;
	Shader::mergeShaderReflections(vertexShader.getReflectionData(), pixelShader.getReflectionData(), resources, constantBuffers);

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

ComPointer<ID3D12RootSignature>& Pipeline::getRootSignature()
{
	return this->rootSignature;
}

DescriptorHeap* Pipeline::getDescriptorHeap()
{
	return descriptorHeap;
}

void Pipeline::releaseResources()
{
	rootSignature.Release();
}