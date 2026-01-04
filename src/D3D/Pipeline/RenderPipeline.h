#pragma once

#include "Pipeline.h"

enum DepthMode {
	DISABLED,
	STANDARD,
	ENVIRONMENT_MAP
};

struct PipelineFormat {
	DepthMode depthMode{ STANDARD };
	DXGI_FORMAT renderTargetFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
};

class RenderPipeline : public Pipeline {
public:
	RenderPipeline() = delete;
	RenderPipeline(std::string vertexShaderName, std::string fragShaderName, DXContext& context, CommandListID id, DescriptorHeap* dh, PipelineFormat pipelineFormat);

	Shader& getVertexShader() { return vertexShader; }
	Shader& getFragmentShader() { return fragShader; }

	void releaseResources() override;
protected:
	void createRootSignature(DXContext& context, std::vector<Shader*> shaders) override;

	void generateRootSignature(DXContext& context, Shader& vertexShader, Shader& pixelShader);

	void createPSOD() override;

	void createPipelineState(ComPointer<ID3D12Device6>& device) override;

	Shader vertexShader, fragShader;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gfxPsod{};

	PipelineFormat pipelineFormat;
};
