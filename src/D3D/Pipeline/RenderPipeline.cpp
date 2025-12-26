#include "RenderPipeline.h"

#include "D3D/ResourceManager.h"

RenderPipeline::RenderPipeline(std::string vertexShaderName, std::string fragShaderName, DXContext& context,
    CommandListID id, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, bool useDsv)
	: Pipeline(context, id), vertexShader(vertexShaderName, ShaderType::VertexShader), fragShader(fragShaderName, ShaderType::PixelShader), useDsv(useDsv)
{
    createRootSignature(context, { &vertexShader, &fragShader });

    ResourceManager& manager = ResourceManager::get(&context);
    descriptorHeap = manager.getDescriptorHeap(manager.createDescriptorHeap(type, numDescriptors, flags));

    createPSOD();
	createPipelineState(context.getDevice());
}

D3D12_INPUT_ELEMENT_DESC vertexLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
        D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
        D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
        D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

void RenderPipeline::createRootSignature(DXContext& context, std::vector<Shader*> shaders)
{
	if (shaders.size() != 2) {
		throw std::runtime_error("RenderPipeline::createRootSignature requires exactly 2 shaders (vertex and pixel)");
	}
	generateRootSignature(context, *shaders[0], *shaders[1]);
}

void RenderPipeline::generateRootSignature(DXContext& context, Shader& vertexShader, Shader& pixelShader) {
    ComPointer<ID3D12Device6> device = context.getDevice();

    RootSignatureBuilder builder;

    std::vector<ShaderResourceBinding> resources;
    std::vector<ConstantBufferReflection> constantBuffers;
    Shader::mergeShaderReflections(vertexShader.getReflectionData(), pixelShader.getReflectionData(), resources, constantBuffers);

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

    builder.build(device.Get(), rootSignature);
}

void RenderPipeline::createPSOD() {
    gfxPsod.pRootSignature = rootSignature;
    gfxPsod.InputLayout.NumElements = _countof(vertexLayout);
    gfxPsod.InputLayout.pInputElementDescs = vertexLayout;
    gfxPsod.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

    gfxPsod.VS.BytecodeLength = vertexShader.getSize();
    gfxPsod.VS.pShaderBytecode = vertexShader.getBuffer();

    gfxPsod.PS.BytecodeLength = fragShader.getSize();
    gfxPsod.PS.pShaderBytecode = fragShader.getBuffer();

    gfxPsod.DS.BytecodeLength = 0;
    gfxPsod.DS.pShaderBytecode = nullptr;
    gfxPsod.HS.BytecodeLength = 0;
    gfxPsod.HS.pShaderBytecode = nullptr;
    gfxPsod.GS.BytecodeLength = 0;
    gfxPsod.GS.pShaderBytecode = nullptr;

    gfxPsod.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    gfxPsod.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    gfxPsod.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    gfxPsod.RasterizerState.FrontCounterClockwise = FALSE;
    gfxPsod.RasterizerState.DepthBias = 0;
    gfxPsod.RasterizerState.DepthBiasClamp = .0f;
    gfxPsod.RasterizerState.SlopeScaledDepthBias = .0f;
    gfxPsod.RasterizerState.DepthClipEnable = FALSE;
    gfxPsod.RasterizerState.MultisampleEnable = FALSE;
    gfxPsod.RasterizerState.AntialiasedLineEnable = FALSE;
    gfxPsod.RasterizerState.ForcedSampleCount = 0;

    gfxPsod.StreamOutput.NumEntries = 0;
    gfxPsod.StreamOutput.NumStrides = 0;
    gfxPsod.StreamOutput.pBufferStrides = nullptr;
    gfxPsod.StreamOutput.pSODeclaration = nullptr;
    gfxPsod.StreamOutput.RasterizedStream = 0;

    gfxPsod.NumRenderTargets = 1;

    gfxPsod.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    if (useDsv) {
        gfxPsod.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        gfxPsod.DepthStencilState.DepthEnable = TRUE;
        gfxPsod.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        gfxPsod.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        gfxPsod.DepthStencilState.StencilEnable = FALSE;
        gfxPsod.DepthStencilState.StencilReadMask = 0;
        gfxPsod.DepthStencilState.StencilWriteMask = 0;
        gfxPsod.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        gfxPsod.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        gfxPsod.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        gfxPsod.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        gfxPsod.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        gfxPsod.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        gfxPsod.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        gfxPsod.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    }
    else {
		gfxPsod.DSVFormat = DXGI_FORMAT_UNKNOWN;
		gfxPsod.DepthStencilState.DepthEnable = FALSE;
        gfxPsod.DepthStencilState.StencilEnable = FALSE;
    }

    gfxPsod.BlendState.AlphaToCoverageEnable = FALSE;
    gfxPsod.BlendState.IndependentBlendEnable = FALSE;
    gfxPsod.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
    gfxPsod.BlendState.RenderTarget[0].BlendEnable = TRUE;
    gfxPsod.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;

    gfxPsod.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    gfxPsod.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    gfxPsod.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
    gfxPsod.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    gfxPsod.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    gfxPsod.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    gfxPsod.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    gfxPsod.SampleMask = 0xFFFFFFFF;

    gfxPsod.SampleDesc.Count = 1;
    gfxPsod.SampleDesc.Quality = 0;

    gfxPsod.NodeMask = 0;

    gfxPsod.CachedPSO.CachedBlobSizeInBytes = 0;
    gfxPsod.CachedPSO.pCachedBlob = nullptr;

    gfxPsod.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void RenderPipeline::createPipelineState(ComPointer<ID3D12Device6>& device) {
	device->CreateGraphicsPipelineState(&gfxPsod, IID_PPV_ARGS(&pso));
}

void RenderPipeline::releaseResources() {
	vertexShader.releaseResources();
	fragShader.releaseResources();
	Pipeline::releaseResources();
}
