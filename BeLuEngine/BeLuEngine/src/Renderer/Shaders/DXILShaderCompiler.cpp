#include "stdafx.h"
#include "DXILShaderCompiler.h"

#include <filesystem>

#include "../Misc/AssetLoader.h"

#include "../API/D3D12/D3D12GraphicsManager.h"

DXILShaderCompiler::DXILShaderCompiler()
{
    m_DxcCompilerDLL = LoadLibraryA("dxcompiler.dll");
    BL_ASSERT_MESSAGE(m_DxcCompilerDLL, "dxcompiler.dll is missing");

    DxcCreateInstanceProc pfnDxcCreateInstance = DxcCreateInstanceProc(GetProcAddress(m_DxcCompilerDLL, "DxcCreateInstance"));

    HRESULT hr = E_FAIL;

    if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler))))
    {
        if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pPreProcessCompiler))))
        {
            if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_pUtils))))
            {
                if (SUCCEEDED(m_pUtils->CreateDefaultIncludeHandler(&m_pDefaultIncludeHandler)))
                {
                    BL_LOG_INFO("DXIL Compiler succesfully created!\n");
                }
            }
        }
    }
}

DXILShaderCompiler::~DXILShaderCompiler()
{
    BL_SAFE_RELEASE(&m_pCompiler);
    BL_SAFE_RELEASE(&m_pPreProcessCompiler);
    BL_SAFE_RELEASE(&m_pUtils);
    BL_SAFE_RELEASE(&m_pDefaultIncludeHandler);

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
    hr = m_pUtils->LoadFile(desc->filePath, &encoding, &blobEncoding);
    
    if (FAILED(hr))
    {
        BL_LOG_CRITICAL("Failed to Load shaderSourceFile: %S\n", desc->filePath);
        BL_ASSERT(false);
        return nullptr;
    }
        
#pragma endregion

#pragma region parseArgumentData
    std::vector<LPCWSTR> compileArguments = {};

    // Entry Point (eg vs_main)
    compileArguments.push_back(L"-E");
    compileArguments.push_back(desc->entryPoint);

    // Target (eg vs_6_5)
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

#pragma region PreProcess

    IDxcOperationResult* opResult = nullptr;
    hr = m_pPreProcessCompiler->Preprocess(blobEncoding, desc->filePath, desc->arguments.data(), (UINT32)desc->arguments.size(), nullptr, 0, m_pDefaultIncludeHandler, &opResult);

    if (FAILED(hr))
    {
        BL_LOG_CRITICAL("Failed to preprocess shader: %S\n", desc->filePath);
        BL_ASSERT(false);
        return nullptr;
    }

    IDxcBlob* preProcessedBlob = nullptr;
    hr = opResult->GetResult(&preProcessedBlob);
    if (FAILED(hr))
    {
        BL_LOG_CRITICAL("Failed to GetPreprocessed result from shader: %S\n", desc->filePath);
        BL_ASSERT(false);
        return nullptr;
    }
    BL_SAFE_RELEASE(&blobEncoding);
    BL_SAFE_RELEASE(&opResult);
#pragma endregion

    DxcBuffer sourceBuffer = {};
    sourceBuffer.Ptr = preProcessedBlob->GetBufferPointer();
    sourceBuffer.Size = preProcessedBlob->GetBufferSize();
    sourceBuffer.Encoding = 0;

    hr = m_pCompiler->Compile(&sourceBuffer, compileArguments.data(), (UINT32)compileArguments.size(), m_pDefaultIncludeHandler, IID_PPV_ARGS(&compiledResult));
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
            BL_LOG_WARNING("Shader Warning//Error Outputs: %s\n", errorString);

            // Add a final warning saying which shader it is that failed
            BL_LOG_WARNING("Failed to compile shader: %S\n", desc->filePath);
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
