#ifndef DXILSHADERCOMPILER_H
#define DXILSHADERCOMPILER_H

#include <Windows.h>
#include <vector>
#include <dxcapi.h>

class DXILShaderCompiler
{
public:
	~DXILShaderCompiler();
	static DXILShaderCompiler* Get();

	struct DXILCompilationDesc
	{
		LPCVOID source = nullptr;
		LPCWSTR filePath;
		LPCWSTR entryPoint;
		LPCWSTR targetProfile;
		std::vector<LPCWSTR> compileArguments;
		std::vector<DxcDefine> defines;
	};

	HRESULT Init();

	// Compiles a shader into a blob
	// Compiles from source if source != nullptr, otherwise from file
	HRESULT CompileFromFile(DXILCompilationDesc* desc, IDxcBlob** ppResult);

private:
	DXILShaderCompiler();
	
	IDxcLibrary* m_pLibrary = nullptr;
	IDxcCompiler2* m_pCompiler = nullptr;
	IDxcLinker* m_pLinker = nullptr;
	IDxcIncludeHandler* m_pIncludeHandler = nullptr;
};

#endif