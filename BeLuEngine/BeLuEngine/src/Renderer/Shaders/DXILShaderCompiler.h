#ifndef DXILSHADERCOMPILER_H
#define DXILSHADERCOMPILER_H

#include "RenderCore.h"

struct IDxcBlob;
struct IDxcCompiler3;
struct IDxcUtils;

struct DXILCompilationDesc
{
	E_SHADER_TYPE shaderType = E_SHADER_TYPE::UNDEFINED;
	LPCWSTR filePath = {};		// Set the local path here (eg. L"vShader.hlsl" instead of L"BeLuEngine/bla/bla/Shaders/vShader.hlsl")
	LPCWSTR entryPoint = {};	// eg.. vs_main, ps_main ...
	LPCWSTR target = {};		// Will be set automatically
	std::vector<LPCWSTR> arguments = {};	// Will be set automatically
	std::vector<LPCWSTR> defines = {};		// set defines here for this shader permutation
};

class DXILShaderCompiler
{
public:
	~DXILShaderCompiler();
	static DXILShaderCompiler& GetInstance();

	// Compiles a shader into a blob
	IDxcBlob* Compile(DXILCompilationDesc* desc);

private:
	DXILShaderCompiler();

	friend class BShaderManager;

	// The DLL
	HMODULE m_DxcCompilerDLL;

	IDxcCompiler3* m_Compiler;

	// Utils
	IDxcUtils* m_Utils;
};

#endif