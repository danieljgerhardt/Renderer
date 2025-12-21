#pragma once
#include "Pipeline.h"

class MeshPipeline : public Pipeline {
public:
	MeshPipeline() = delete;
	MeshPipeline(std::string meshShaderName, std::string fragShaderName, DXContext& context,
		CommandListID cmdID, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

	Shader& getMeshShader() { return meshShader; }

private: 
	void createPSOD() override;
	void createPipelineState(ComPointer<ID3D12Device6>& device) override;
	void createRootSignature(DXContext& context, std::vector<Shader*> shaders) override;
	void generateRootSignature(DXContext& context, Shader& meshShader, Shader& pixelShader);

	Shader meshShader, fragShader;

	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psod{};
};