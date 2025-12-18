#pragma once

#include "../Support/WinInclude.h"

#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <unordered_map>

#include "ComPointer.h"

enum class ShaderResourceType
{
    ConstantBuffer,
    RootConstant,
    Texture,
    Sampler,
    RWTexture,
    StructuredBuffer,
    RWStructuredBuffer
};

struct ShaderResourceBinding
{
    std::string name;
    ShaderResourceType type;
    D3D12_SHADER_VISIBILITY visibility;
    UINT bindPoint;
    UINT bindCount;
    UINT space;
    UINT size; // For constant buffers
};

enum class ParameterType
{
    Int = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Matrix4x4,
    Count,
    Invalid
};

struct ParameterDesc
{
    ParameterDesc() = default;
    //ParameterDesc(const char* name, ParameterType type);
    std::string name;
    ParameterType type = ParameterType::Invalid;
    UINT index = 0;
    UINT offset = 0;
    std::string constantBufferName; // Which CB this belongs to
};

struct ConstantBufferReflection
{
    std::string name;
    UINT bindPoint;
    UINT space;
    UINT size;
    std::vector<ParameterDesc> variables;
	bool isRootConstant = false;
};

enum class Semantics : uint8_t
{
    POSITION,
    NORMAL,
    TEXCOORD,
    TANGENT,
    BINORMAL,
    COLOR,
    BLENDINDICES,
    BLENDWEIGHTS,
    WORLDMATRIX,
    COUNT
};

struct VertexBufferDescription
{
    Semantics* SemanticsArr = nullptr;
    uint16_t* ByteOffsets = nullptr;
    uint16_t   AttrCount = 0;
    uint16_t   ByteSize = 0;
};

struct ShaderReflectionData
{
    std::vector<ShaderResourceBinding> resources;
    std::vector<ConstantBufferReflection> constantBuffers;
    bool isReflected = false;
};

enum ShaderType {
	VertexShader = 0,
	PixelShader,
	ComputeShader,
    MeshShader,
    RootSigOrUnassigned
};

class Shader
{
public:
    Shader() = delete;
    Shader(std::string_view name, ShaderType type);
    inline const void* getBuffer() const { return data; }
    inline size_t getSize() const { return size; }

	ShaderReflectionData& getReflectionData() { return reflectionData; }

	void releaseResources();
private:
    void* data = nullptr;
    size_t size = 0;

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderBlob;
	Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection;

	ShaderReflectionData reflectionData;

	ShaderType shaderType;

public:
    static void mergeShaderReflections(ShaderReflectionData& vsData, ShaderReflectionData& psData, std::vector<ShaderResourceBinding>& outResources, std::vector<ConstantBufferReflection>& outConstants) {
        outResources.clear();
        outConstants.clear();

        std::unordered_map<std::string, size_t> nameToIndex;

        auto mergeResources = [](ShaderResourceBinding& dst, const ShaderResourceBinding& src)
            {
                if (dst.visibility != src.visibility)
                    dst.visibility = D3D12_SHADER_VISIBILITY_ALL;
            };

        for (const auto& res : vsData.resources) {
            nameToIndex[res.name] = outResources.size();
            outResources.push_back(res);
        }

        for (const auto& res : psData.resources) {
            if (nameToIndex.find(res.name) != nameToIndex.end()) {
                size_t idx = nameToIndex[res.name];
                ShaderResourceBinding& dst = outResources[idx];
                mergeResources(dst, res);
                continue;
            }

            outResources.push_back(res);
        }

        nameToIndex.clear();
        for (const auto& cb : vsData.constantBuffers) {
            outConstants.push_back(cb);
            nameToIndex[cb.name] = outConstants.size() - 1;
        }

        for (const auto& constant : psData.constantBuffers) {
            auto it = nameToIndex.find(constant.name);
            if (it != nameToIndex.end())
            {
                // CB exists in both shaders - merge variables
                auto& existingCB = outConstants[it->second];

                // Verify same size and binding
                if (existingCB.size != constant.size ||
                    existingCB.bindPoint != constant.bindPoint ||
                    existingCB.space != constant.space)
                {
                    printf("Warning: Constant buffer '%s' has different properties in VS and PS!\n", constant.name.c_str());
                }


                // Merge variables (avoid duplicates)
                for (const auto& var : constant.variables)
                {
                    // TODO: This feels a bit slow
                    bool found = false;
                    for (const auto& existingVar : existingCB.variables)
                    {
                        if (existingVar.name == var.name)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        existingCB.variables.push_back(var);
                }
            }
            else
            {
                // New CB only in pixel shader
                outConstants.push_back(constant);
                nameToIndex[constant.name] = outConstants.size() - 1;
            }
        }
    }
};