#pragma once

#include <vector>
#include <array>
#include "Support/WinInclude.h"
#include "Support/ComPointer.h"

class RootSignatureBuilder
{
public:
    void reset();
    void addConstantBufferView(UINT shaderRegister, UINT space, D3D12_SHADER_VISIBILITY visibility);
    void addShaderResourceView(UINT shaderRegister, UINT space, D3D12_SHADER_VISIBILITY visibility);
    void addUnorderedAccessView(UINT shaderRegister, UINT space, D3D12_SHADER_VISIBILITY visibility);
    void addDescriptorTable(const D3D12_DESCRIPTOR_RANGE* ranges, UINT numRanges, D3D12_SHADER_VISIBILITY visibility);
	void addRootConstant(UINT shaderRegister, UINT space, UINT num32BitValues, D3D12_SHADER_VISIBILITY visibility);
    void addStaticSampler(UINT shaderRegister, UINT registerSpace);

    void build(ID3D12Device* pDevice, ComPointer<ID3D12RootSignature>& rootSig);

private:
    std::vector<D3D12_ROOT_PARAMETER> parameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorRanges;

    static const size_t NUM_STATIC_SAMPLERS = 6;
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, NUM_STATIC_SAMPLERS> initStaticSamplers()
    {
        const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
            0, // shaderRegister
            D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

        const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
            1, // shaderRegister
            D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

        const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
            2, // shaderRegister
            D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

        const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
            3, // shaderRegister
            D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

        const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
            4, // shaderRegister
            D3D12_FILTER_ANISOTROPIC, // filter
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
            0.0f,                             // mipLODBias
            8);                               // maxAnisotropy

        const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
            5, // shaderRegister
            D3D12_FILTER_ANISOTROPIC, // filter
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
            0.0f,                              // mipLODBias
            8);                                // maxAnisotropy

        return {
            pointWrap, pointClamp,
            linearWrap, linearClamp,
            anisotropicWrap, anisotropicClamp };
    }
};

