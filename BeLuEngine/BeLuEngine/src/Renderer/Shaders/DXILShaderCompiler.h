#ifndef DXILSHADERCOMPILER_H
#define DXILSHADERCOMPILER_H

#include "RenderCore.h"

struct IDxcBlob;
struct IDxcCompiler3;
struct IDxcUtils;

struct DXILCompilationDesc
{
	E_SHADER_TYPE shaderType = E_SHADER_TYPE::UNDEFINED;
	LPCWSTR filePath = L"";		// Set the local path here (eg. L"vShader.hlsl" instead of L"BeLuEngine/bla/bla/Shaders/vShader.hlsl")
	LPCWSTR entryPoint = L"";	// eg.. vs_main, ps_main ...
	LPCWSTR target = L"";		// Will be set automatically
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

	// The DLL
	HMODULE m_DxcCompilerDLL = nullptr;

	// This is really hacky.. but 3 is not inheriting from 2 -> 1 -> 0 etc..
	IDxcCompiler* m_pPreProcessCompiler = nullptr;
	IDxcCompiler3* m_pCompiler = nullptr;

	// Utils
	IDxcUtils* m_pUtils = nullptr;
	IDxcIncludeHandler* m_pDefaultIncludeHandler = {};
};

#endif