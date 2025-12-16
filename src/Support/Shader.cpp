#include "Shader.h"

Shader::Shader(std::string_view name, ShaderType type) {
    //TODO - find a way to link compiler dll without needing it in build folder
    shaderType = type;

    static std::filesystem::path shaderDir;

    if (shaderDir.empty()) {
        wchar_t moduleFileName[MAX_PATH];
        GetModuleFileNameW(nullptr, moduleFileName, MAX_PATH);
        shaderDir = moduleFileName;
        shaderDir.remove_filename();
    }

    std::ifstream shaderIn(shaderDir / name, std::ios::binary);

    if (shaderIn.is_open()) {
        shaderIn.seekg(0, std::ios::end);
        size = shaderIn.tellg();
        shaderIn.seekg(0, std::ios::beg);
        data = malloc(size);
        if (data) {
            shaderIn.read((char*)data, size);
        }
    }

	if (type == ShaderType::RootSigOrUnassigned) {
		return;
	}

    //start reflection

    ComPointer<IDxcLibrary> pLibrary;
    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&pLibrary));
    assert(SUCCEEDED(hr));

    std::filesystem::path fullPath = shaderDir / name;
    std::wstring wname = fullPath.wstring();
    
    hr = pLibrary->CreateBlobFromFile(wname.c_str(), nullptr, shaderBlob.GetAddressOf());
    assert(SUCCEEDED(hr));

    //reflect
	Microsoft::WRL::ComPtr<IDxcContainerReflection> pContainerReflection;
	hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&pContainerReflection));
	assert(SUCCEEDED(hr));

	hr = pContainerReflection->Load(shaderBlob.Get());
	assert(SUCCEEDED(hr));

	UINT32 codeIndex;
	hr = pContainerReflection->FindFirstPartKind(DXC_PART_DXIL, &codeIndex);
	assert(SUCCEEDED(hr));

	hr = pContainerReflection->GetPartReflection(codeIndex, IID_PPV_ARGS(&shaderReflection));
	assert(SUCCEEDED(hr));

    //at this point reflection has succeeded so we parse the necessary info

    D3D12_SHADER_DESC shaderDesc;
	shaderReflection->GetDesc(&shaderDesc);

    for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
		D3D12_SHADER_INPUT_BIND_DESC bindDesc;
		shaderReflection->GetResourceBindingDesc(i, &bindDesc);

        ShaderResourceBinding resource;
		resource.name = bindDesc.Name;
		resource.bindPoint = bindDesc.BindPoint;
		resource.bindCount = bindDesc.BindCount;
		resource.space = bindDesc.Space;
        switch (shaderType) {
		case ShaderType::VertexShader:
			resource.visibility = D3D12_SHADER_VISIBILITY_VERTEX;
			break;
		case ShaderType::PixelShader:
			resource.visibility = D3D12_SHADER_VISIBILITY_PIXEL;
			break;
        default:
			resource.visibility = D3D12_SHADER_VISIBILITY_ALL;
			break;
        }
        resource.size = 0;

		switch (bindDesc.Type) {
        case D3D_SIT_CBUFFER:
            //check if resource.name starts with rc_ or cbv_
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
        default:
            printf("Warning: Found unsupported shader resource: %s\n", resource.name.c_str());
            continue;
        }

        if (resource.type == ShaderResourceType::ConstantBuffer || resource.type == ShaderResourceType::RootConstant)
        {
            ID3D12ShaderReflectionConstantBuffer* pCBReflection =
                shaderReflection->GetConstantBufferByName(bindDesc.Name);

            D3D12_SHADER_BUFFER_DESC cbDesc;
            pCBReflection->GetDesc(&cbDesc);

            ConstantBufferReflection cbReflection;
            cbReflection.name = bindDesc.Name;
            cbReflection.bindPoint = bindDesc.BindPoint;
            cbReflection.space = bindDesc.Space;
            cbReflection.size = cbDesc.Size;
			cbReflection.isRootConstant = (resource.type == ShaderResourceType::RootConstant);

            //parse variables
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
                        case 2: param.type = ParameterType::Float2;
                        case 3: param.type = ParameterType::Float3;
                        case 4: param.type = ParameterType::Float4;
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

            reflectionData.constantBuffers.push_back(cbReflection);

            //update resource size
            resource.size = cbDesc.Size;
        }
        reflectionData.resources.push_back(resource);
    }

	reflectionData.isReflected = true;
    
    //data has been parsed

    //if shader is vert shader, handle input layout

}

Shader::~Shader() {
    if (data) {
        free(data);
    }
}