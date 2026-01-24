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
		&closesthitSource
	);

	utils->LoadFile(
		(closestHitShaderPath).wstring().c_str(),
		nullptr,
		&missSource
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

    ComPointer<IDxcBlob> dxil;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxil), nullptr);

    data = malloc(dxil->GetBufferSize());
    size = dxil->GetBufferSize();
    memcpy(data, dxil->GetBufferPointer(), size);
}

void ShaderLib::releaseResources() {
    if (data) {
	    free(data);
	    data = nullptr;
	    size = 0;
    }
}
