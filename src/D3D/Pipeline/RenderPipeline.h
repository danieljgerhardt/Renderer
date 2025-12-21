#pragma once
#include "Pipeline.h"

class RenderPipeline : public Pipeline {
public:
	RenderPipeline() = delete;
	RenderPipeline(std::string vertexShaderName, std::string fragShaderName, DXContext& context,
		CommandListID id, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

	Shader& getVertexShader() { return vertexShader; }
	Shader& getFragmentShader() { return fragShader; }

	void releaseResources() override;
private:
	void createRootSignature(DXContext& context, std::vector<Shader*> shaders) override;

	void generateRootSignature(DXContext& context, Shader& vertexShader, Shader& pixelShader);

	void createPSOD() override;

	void createPipelineState(ComPointer<ID3D12Device6>& device) override;

	Shader vertexShader, fragShader;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPsod{};
};
