#include "stdafx.h"
#include "DXILShaderCompiler.h"

DXILShaderCompiler::DXILShaderCompiler()
{
}

DXILShaderCompiler::~DXILShaderCompiler() 
{
	BL_SAFE_RELEASE(&m_pCompiler);
	BL_SAFE_RELEASE(&m_pLibrary);
	BL_SAFE_RELEASE(&m_pIncludeHandler);
	BL_SAFE_RELEASE(&m_pLinker);
}

DXILShaderCompiler* DXILShaderCompiler::Get()
{
	static DXILShaderCompiler instance;
	
	return &instance;
}

HRESULT DXILShaderCompiler::Init() 
{

	HMODULE dll = LoadLibraryA("dxcompiler.dll");
	if (dll == false)
	{
		BL_LOG_CRITICAL("dxcompiler.dll is missing");
	}

	DxcCreateInstanceProc pfnDxcCreateInstance = DxcCreateInstanceProc(GetProcAddress(dll, "DxcCreateInstance"));

	HRESULT hr = E_FAIL;

	if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler))))
	{
		if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_pLibrary))))
		{
			if (SUCCEEDED(m_pLibrary->CreateIncludeHandler(&m_pIncludeHandler)))
			{
				if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&m_pLinker))))
				{
					BL_LOG_INFO("DXIL Compiler succesfully created!\n");
				}
			}
		}
	}

	return hr;
}

HRESULT DXILShaderCompiler::CompileFromFile(DXILCompilationDesc* desc, IDxcBlob** ppResult) {
	HRESULT hr = E_FAIL;

	if (desc != nullptr)
	{
		IDxcBlobEncoding* source = nullptr;

		// Compile from file path
		hr = m_pLibrary->CreateBlobFromFile(desc->filePath, nullptr, &source);

		if (SUCCEEDED(hr))
		{
			IDxcOperationResult* pResult = nullptr;

#ifdef DEBUG
			IDxcBlob* debugBlob = nullptr;
			LPWSTR a = L"debugBlobName";
			hr = m_pCompiler->CompileWithDebug(
				source,									// program text
				desc->filePath,							// file name, mostly for error messages
				desc->entryPoint,						// entry point function
				desc->targetProfile,					// target profile
				desc->compileArguments.data(),          // compilation arguments
				(UINT)desc->compileArguments.size(),	// number of compilation arguments
				desc->defines.data(),					// define arguments
				(UINT)desc->defines.size(),				// number of define arguments
				m_pIncludeHandler,						// handler for #include directives
				&pResult,
				&a,										// DebugBlobName
				&debugBlob);							// DebugBlob

			if (FAILED(hr) && debugBlob != nullptr)
			{
				BL_LOG_CRITICAL("Failed to compile shader: %S\n", desc->filePath);

				const char* errorMsg = static_cast<const char*>(debugBlob->GetBufferPointer());
				BL_LOG_CRITICAL("%s\n", errorMsg);
			}
#else
			hr = m_pCompiler->Compile(
				source,									// program text
				desc->filePath,							// file name, mostly for error messages
				desc->entryPoint,						// entry point function
				desc->targetProfile,					// target profile
				desc->compileArguments.data(),          // compilation arguments
				(UINT)desc->compileArguments.size(),	// number of compilation arguments
				desc->defines.data(),					// define arguments
				(UINT)desc->defines.size(),				// number of define arguments
				m_pIncludeHandler,						// handler for #include directives
				&pResult);
#endif
			if (SUCCEEDED(hr))
			{
				HRESULT hrCompile = E_FAIL;
				if (SUCCEEDED(hr = pResult->GetStatus(&hrCompile)))
				{
					if (SUCCEEDED(hrCompile))
					{
						if (ppResult)
						{
							pResult->GetResult(ppResult);
							hr = S_OK;
						} 
						else 
						{
							hr = E_FAIL;
						}
					} 
					else 
					{
						IDxcBlobEncoding* pPrintBlob = nullptr;
						if (SUCCEEDED(pResult->GetErrorBuffer(&pPrintBlob)))
						{
							// We can use the library to get our preferred encoding.
							IDxcBlobEncoding* pPrintBlob16 = nullptr;
							m_pLibrary->GetBlobAsUtf16(pPrintBlob, &pPrintBlob16);



							std::wstring b = (LPCWSTR)pPrintBlob16->GetBufferPointer();
							std::string a = to_string(b);
							hr = hrCompile;

							BL_LOG_CRITICAL("%s\n", a.c_str());


							BL_SAFE_RELEASE(&pPrintBlob);
							BL_SAFE_RELEASE(&pPrintBlob16);
						}
					}
					BL_SAFE_RELEASE(&pResult);
				}
			}
		}
	}
	return hr;
}
