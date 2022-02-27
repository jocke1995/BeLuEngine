#include "stdafx.h"
#include "DXILShaderCompiler.h"

#include "../API/D3D12/D3D12GraphicsManager.h"

DXILShaderCompiler::DXILShaderCompiler()
{
    m_DxcCompilerDLL = LoadLibraryA("dxcompiler.dll");
    BL_ASSERT_MESSAGE(m_DxcCompilerDLL, "dxcompiler.dll is missing");

    DxcCreateInstanceProc pfnDxcCreateInstance = DxcCreateInstanceProc(GetProcAddress(m_DxcCompilerDLL, "DxcCreateInstance"));

    HRESULT hr = E_FAIL;

    if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler))))
    {
        if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils))))
        {
            BL_LOG_INFO("DXIL Compiler succesfully created!\n");
        }
    }
}

DXILShaderCompiler::~DXILShaderCompiler()
{
    BL_SAFE_RELEASE(&m_Compiler);
    BL_SAFE_RELEASE(&m_Utils);

    FreeLibrary(m_DxcCompilerDLL);
    m_DxcCompilerDLL = nullptr;
}

DXILShaderCompiler& DXILShaderCompiler::GetInstance()
{
    static DXILShaderCompiler instance;

    return instance;
}


IDxcBlob* DXILShaderCompiler::Compile(DXILCompilationDesc* desc)
{
    HRESULT hr = E_FAIL;

    IDxcResult* compiledResult = nullptr;
    IDxcBlob* strippedResult = nullptr;

#pragma region prepareSourceblob
    IDxcBlobEncoding* blobEncoding = {};
    unsigned int encoding = 0;
    hr = m_Utils->LoadFile(desc->filePath, &encoding, &blobEncoding);
    
    if (FAILED(hr))
    {
        BL_LOG_CRITICAL("Failed to Load shaderSourceFile: %S\n", desc->filePath);
        BL_ASSERT(false);
        return nullptr;
    }

    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr = blobEncoding->GetBufferPointer();
    sourceBuffer.Size = blobEncoding->GetBufferSize();
    sourceBuffer.Encoding = 0;
        
#pragma endregion

    std::vector<LPCWSTR> compileArguments = {};
#pragma region parseArgumentData
    // Entry Point (eg vs_6_5)
    compileArguments.push_back(L"-E");
    compileArguments.push_back(desc->entryPoint);

    // Target (eg vs_main
    compileArguments.push_back(L"-T");
    compileArguments.push_back(desc->target);

    // Compiler Arguments (eg -Zi, -WX ..)
    for (const LPCWSTR& arg : desc->arguments)
    {
        compileArguments.push_back(arg);
    }

    // Defines (eg DEBUG, HIGHQUALITY, OTHER_SHADER_SPECIFIC_STUFF ..)
    for (const LPCWSTR& arg : desc->defines)
    {
        compileArguments.push_back(L"-D");
        compileArguments.push_back(arg);
    }
#pragma endregion
    hr = m_Compiler->Compile(&sourceBuffer, compileArguments.data(), (UINT32)compileArguments.size(), nullptr, IID_PPV_ARGS(&compiledResult));
    if (FAILED(hr))
    {
        BL_LOG_CRITICAL("Failed to compile shader: %S\n", desc->filePath);
        BL_ASSERT(false);
        return nullptr;
    }

    IDxcBlobUtf8* pErrorBuffer = nullptr;
    hr = compiledResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrorBuffer), nullptr);
    if (SUCCEEDED(hr))
    {
        // Is the printBlob empty?
        if (pErrorBuffer && pErrorBuffer->GetStringLength() > 0)
        {
            const char* errorString = pErrorBuffer->GetStringPointer();

            // Print warnings and errors
            BL_LOG_CRITICAL("Shader Error Outputs: %s\n", errorString);

            // Add a final warning saying which shader it is that failed
            BL_LOG_CRITICAL("Failed to compile shader: %S\n", desc->filePath);
        }
    }
    BL_SAFE_RELEASE(&pErrorBuffer);

    // Get a stripped version of the final ShaderBlob
    if (SUCCEEDED(hr))
    {
        hr = compiledResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&strippedResult), nullptr);

        if (FAILED(hr))
        {
            BL_LOG_CRITICAL("Failed to strip shader: %S\n", desc->filePath);
        }
    }

    BL_SAFE_RELEASE(&compiledResult);

    return strippedResult;
}
