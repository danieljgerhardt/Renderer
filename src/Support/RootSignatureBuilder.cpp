#include "RootSignatureBuilder.h"

void RootSignatureBuilder::reset()
{
    parameters.clear();
    staticSamplers.clear();
    descriptorRanges.clear();
}

void RootSignatureBuilder::addConstantBufferView(UINT shaderRegister, UINT space, D3D12_SHADER_VISIBILITY visibility)
{
    D3D12_ROOT_PARAMETER cbv = {};
    cbv.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    cbv.ShaderVisibility = visibility;
    cbv.Descriptor.ShaderRegister = shaderRegister;
    cbv.Descriptor.RegisterSpace = space;
    parameters.push_back(cbv);
}

void RootSignatureBuilder::addShaderResourceView(UINT shaderRegister, UINT space, D3D12_SHADER_VISIBILITY visibility)
{
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = shaderRegister;
    srvRange.RegisterSpace = space;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    addDescriptorTable(&srvRange, 1, visibility);
}

void RootSignatureBuilder::addUnorderedAccessView(UINT shaderRegister, UINT space, D3D12_SHADER_VISIBILITY visibility)
{
    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = shaderRegister;
    uavRange.RegisterSpace = space;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    addDescriptorTable(&uavRange, 1, visibility);
}

void RootSignatureBuilder::addDescriptorTable(const D3D12_DESCRIPTOR_RANGE* ranges, UINT numRanges, D3D12_SHADER_VISIBILITY visibility)
{
    descriptorRanges.emplace_back(ranges, ranges + numRanges);

    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = visibility;
    param.DescriptorTable.NumDescriptorRanges = numRanges;
    param.DescriptorTable.pDescriptorRanges = descriptorRanges.back().data();
    parameters.push_back(param);
}

void RootSignatureBuilder::addRootConstant(UINT shaderRegister, UINT space, UINT num32BitValues, D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_ROOT_PARAMETER rootConst = {};
	rootConst.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootConst.ShaderVisibility = visibility;
	rootConst.Constants.Num32BitValues = num32BitValues;
	rootConst.Constants.ShaderRegister = shaderRegister;
	rootConst.Constants.RegisterSpace = space;
	parameters.push_back(rootConst);
}

void RootSignatureBuilder::addStaticSampler(UINT shaderRegister, UINT registerSpace)
{
    static std::array<const CD3DX12_STATIC_SAMPLER_DESC, NUM_STATIC_SAMPLERS> sStaticSamplerTemplates = initStaticSamplers();

    //todo - keep an eye on this
    UINT index = DirectX::XMMin(shaderRegister, static_cast<UINT>(NUM_STATIC_SAMPLERS - 1));

    CD3DX12_STATIC_SAMPLER_DESC desc = sStaticSamplerTemplates.at(index);
    desc.ShaderRegister = shaderRegister;
    desc.RegisterSpace = registerSpace;
    desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // Assumed visible to all shaders.

    staticSamplers.push_back(desc);
}

void RootSignatureBuilder::build(ID3D12Device* pDevice, ComPointer<ID3D12RootSignature>& rootSig)
{
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = static_cast<UINT>(parameters.size());
    rootSigDesc.pParameters = parameters.data();
    rootSigDesc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
    rootSigDesc.pStaticSamplers = staticSamplers.data();
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &pSignatureBlob,
        &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        return;
    }

    hr = pDevice->CreateRootSignature(
        0,
        pSignatureBlob->GetBufferPointer(),
        pSignatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSig));

    assert(SUCCEEDED(hr));
}