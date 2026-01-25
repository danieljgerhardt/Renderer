#include "ShaderLib.h"

ShaderLib::ShaderLib(std::string_view raygenName, std::string_view missName, std::string_view closesthitName) {
    static std::filesystem::path shaderDir;

    if (shaderDir.empty()) {
        wchar_t moduleFileName[MAX_PATH];
        GetModuleFileNameW(nullptr, moduleFileName, MAX_PATH);
        shaderDir = moduleFileName;
        shaderDir.remove_filename();
    }

    ComPointer<IDxcUtils> utils;
    ComPointer<IDxcCompiler3> compiler;

    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    assert(SUCCEEDED(hr));

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    assert(SUCCEEDED(hr));

    ComPointer<IDxcBlobEncoding> raygenSource, closesthitSource, missSource;

    static std::filesystem::path raygenShaderPath = shaderDir.parent_path().parent_path().parent_path().parent_path() / "src\\Shaders\\PT\\" / raygenName;
	static std::filesystem::path missShaderPath = shaderDir.parent_path().parent_path().parent_path().parent_path() / "src\\Shaders\\PT\\" / missName;
	static std::filesystem::path closestHitShaderPath = shaderDir.parent_path().parent_path().parent_path().parent_path() / "src\\Shaders\\PT\\" / closesthitName;
    static std::filesystem::path includeShaderLoc = raygenShaderPath.parent_path();

    utils->LoadFile(
        (raygenShaderPath).wstring().c_str(),
        nullptr,
        &raygenSource
    );

	utils->LoadFile(
		(missShaderPath).wstring().c_str(),
		nullptr,
		&missSource
	);

	utils->LoadFile(
		(closestHitShaderPath).wstring().c_str(),
		nullptr,
		&closesthitSource
	);

    std::string combinedSource;
    combinedSource.append(static_cast<const char*>(raygenSource->GetBufferPointer()), raygenSource->GetBufferSize());
    combinedSource.append("\n");
    combinedSource.append(static_cast<const char*>(missSource->GetBufferPointer()), missSource->GetBufferSize());
    combinedSource.append("\n");
    combinedSource.append(static_cast<const char*>(closesthitSource->GetBufferPointer()), closesthitSource->GetBufferSize());

    DxcBuffer buffer{
        .Ptr = combinedSource.data(),
        .Size = combinedSource.size(),
        .Encoding = DXC_CP_UTF8
    };

    const wchar_t* args[] =
    {
        L"-T", L"lib_6_6",
        L"-I", includeShaderLoc.c_str(),
        L"-Zi",
        L"-Qembed_debug"
    };

    ComPointer<IDxcIncludeHandler> includeHandler;
    hr = utils->CreateDefaultIncludeHandler(&includeHandler);
    assert(SUCCEEDED(hr));

    ComPointer<IDxcResult> result;
    compiler->Compile(
        &buffer,
        args,
        _countof(args),
        includeHandler,
        IID_PPV_ARGS(&result)
    );

    ComPointer<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

    if (errors && errors->GetStringLength() > 0)
    {
        OutputDebugStringA(errors->GetStringPointer());
        throw std::runtime_error("DXC compilation failed");
    }

    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxil), nullptr);

    data = malloc(dxil->GetBufferSize());
    size = dxil->GetBufferSize();
    memcpy(data, dxil->GetBufferPointer(), size);

    //REFLECTION

    ComPointer<IDxcLibrary> pLibrary;
    hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary));
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<IDxcContainerReflection> pContainerReflection;
    hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&pContainerReflection));
    assert(SUCCEEDED(hr));

    hr = pContainerReflection->Load(dxil.Get());
    assert(SUCCEEDED(hr));

    ComPointer<ID3D12LibraryReflection> libraryReflection;
    UINT32 dxilIndex;
    hr = pContainerReflection->FindFirstPartKind(DXC_PART_DXIL, &dxilIndex);
    assert(SUCCEEDED(hr));

    hr = pContainerReflection->GetPartReflection(dxilIndex, IID_PPV_ARGS(&libraryReflection));
    assert(SUCCEEDED(hr));

    // Deduplicate resources by name
    std::unordered_map<std::string, size_t> resourceNameToIndex;
    std::unordered_map<std::string, size_t> cbNameToIndex;

    // Now enumerate functions in the library
    D3D12_LIBRARY_DESC libDesc;
    libraryReflection->GetDesc(&libDesc);

    for (UINT i = 0; i < libDesc.FunctionCount; ++i) {
        ID3D12FunctionReflection* funcReflection = libraryReflection->GetFunctionByIndex(i);

        D3D12_FUNCTION_DESC funcDesc;
        funcReflection->GetDesc(&funcDesc);

        // Iterate through bound resources for this function
        for (UINT j = 0; j < funcDesc.BoundResources; ++j) {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
            funcReflection->GetResourceBindingDesc(j, &bindDesc);

            // Check if we've already added this resource
            if (resourceNameToIndex.find(bindDesc.Name) != resourceNameToIndex.end()) {
                continue; // Skip duplicates
            }

            ShaderResourceBinding resource;
            resource.name = bindDesc.Name;
            resource.bindPoint = bindDesc.BindPoint;
            resource.bindCount = bindDesc.BindCount;
            resource.space = bindDesc.Space;
            resource.visibility = D3D12_SHADER_VISIBILITY_ALL; // DXR always uses ALL
            resource.size = 0;

            switch (bindDesc.Type) {
            case D3D_SIT_CBUFFER:
                // Check if resource.name starts with rc_
                if (strncmp(resource.name.c_str(), "rc_", 3) == 0) {
                    resource.type = ShaderResourceType::RootConstant;
                }
                else {
                    resource.type = ShaderResourceType::ConstantBuffer;
                }
                break;
            case D3D_SIT_TEXTURE:
                resource.type = ShaderResourceType::Texture;
                break;
            case D3D_SIT_SAMPLER:
                resource.type = ShaderResourceType::Sampler;
                break;
            case D3D_SIT_UAV_RWTYPED:
                resource.type = ShaderResourceType::RWTexture;
                break;
            case D3D_SIT_STRUCTURED:
                resource.type = ShaderResourceType::StructuredBuffer;
                break;
            case D3D_SIT_UAV_RWSTRUCTURED:
                resource.type = ShaderResourceType::RWStructuredBuffer;
                break;
            case D3D_SIT_RTACCELERATIONSTRUCTURE:
                resource.type = ShaderResourceType::AccelerationStructure;
                break;
            default:
                printf("Warning: Found unsupported shader resource: %s\n", resource.name.c_str());
                continue;
            }

            if (resource.type == ShaderResourceType::ConstantBuffer || resource.type == ShaderResourceType::RootConstant)
            {
                // Check if we've already processed this constant buffer
                if (cbNameToIndex.find(bindDesc.Name) == cbNameToIndex.end()) {
                    ID3D12ShaderReflectionConstantBuffer* pCBReflection =
                        funcReflection->GetConstantBufferByName(bindDesc.Name);

                    D3D12_SHADER_BUFFER_DESC cbDesc;
                    pCBReflection->GetDesc(&cbDesc);

                    ConstantBufferReflection cbReflection;
                    cbReflection.name = bindDesc.Name;
                    cbReflection.bindPoint = bindDesc.BindPoint;
                    cbReflection.space = bindDesc.Space;
                    cbReflection.size = cbDesc.Size;
                    cbReflection.isRootConstant = (resource.type == ShaderResourceType::RootConstant);

                    // Parse variables
                    for (UINT v = 0; v < cbDesc.Variables; ++v)
                    {
                        ID3D12ShaderReflectionVariable* pVar = pCBReflection->GetVariableByIndex(v);
                        D3D12_SHADER_VARIABLE_DESC varDesc;
                        pVar->GetDesc(&varDesc);

                        ID3D12ShaderReflectionType* pType = pVar->GetType();
                        D3D12_SHADER_TYPE_DESC typeDesc;
                        pType->GetDesc(&typeDesc);

                        ParameterDesc param;
                        param.name = varDesc.Name;
                        param.offset = varDesc.StartOffset;

                        if (typeDesc.Class == D3D_SVC_SCALAR)
                        {
                            if (typeDesc.Type == D3D_SVT_FLOAT)
                                param.type = ParameterType::Float;
                            else if (typeDesc.Type == D3D_SVT_INT)
                                param.type = ParameterType::Int;
                        }
                        else if (typeDesc.Class == D3D_SVC_VECTOR)
                        {
                            if (typeDesc.Type == D3D_SVT_FLOAT)
                            {
                                switch (typeDesc.Columns)
                                {
                                case 2: param.type = ParameterType::Float2; break;
                                case 3: param.type = ParameterType::Float3; break;
                                case 4: param.type = ParameterType::Float4; break;
                                }
                            }
                        }
                        else if (typeDesc.Class == D3D_SVC_MATRIX_COLUMNS || typeDesc.Class == D3D_SVC_MATRIX_ROWS)
                        {
                            if (typeDesc.Rows == 4 && typeDesc.Columns == 4)
                                param.type = ParameterType::Matrix4x4;
                        }

                        param.constantBufferName = bindDesc.Name;

                        if (param.type != ParameterType::Invalid)
                            cbReflection.variables.push_back(param);
                    }

                    cbNameToIndex[bindDesc.Name] = reflectionData.constantBuffers.size();
                    reflectionData.constantBuffers.push_back(cbReflection);

                    // Update resource size
                    resource.size = cbDesc.Size;
                }
            }

            resourceNameToIndex[bindDesc.Name] = reflectionData.resources.size();
            reflectionData.resources.push_back(resource);
        }
    }

    reflectionData.isReflected = true;
}

void ShaderLib::releaseResources() {
    if (data) {
	    free(data);
	    data = nullptr;
	    size = 0;
    }
}
