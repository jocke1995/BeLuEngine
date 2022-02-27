#include "stdafx.h"
#include "Shader.h"

#include "DXC/dxcapi.h"

#include "DXILShaderCompiler.h"

Shader::Shader(DXILCompilationDesc* desc)
{
	compileShader(desc);
}

Shader::~Shader()
{
	BL_SAFE_RELEASE(&m_pBlob);
}

IDxcBlob* Shader::GetBlob() const
{
	return m_pBlob;
}

void Shader::compileShader(DXILCompilationDesc* desc)
{
	desc->arguments.push_back(L"-Gis"); // ? floating point accuracy?
	desc->arguments.push_back(L"-WX"); // DXC_ARG_WARNINGS_ARE_ERRORS
#ifdef DEBUG
	desc->arguments.push_back(L"/Zi"); // Debug info
	desc->defines.push_back({ L"DEBUG" });
#else
	desc->arguments.push_back(L"-O3"); // DXC_ARG_OPTIMIZATION_LEVEL3
	desc->defines.push_back({ L"NDEBUG" });
#endif

	bool overrideEntryPoint = desc->entryPoint == L"" ? false : true;

	if (desc->shaderType == E_SHADER_TYPE::VS)
	{
		if(overrideEntryPoint == false)
			desc->entryPoint = L"VS_main";

		desc->target = L"vs_6_5";
		desc->defines.push_back({ L"VERTEX_SHADER" });
	}
	else if (desc->shaderType == E_SHADER_TYPE::PS)
	{
		if (overrideEntryPoint == false)
			desc->entryPoint = L"PS_main";

		desc->target = L"ps_6_5";
		desc->defines.push_back({ L"PIXEL_SHADER" });
	}
	else if (desc->shaderType == E_SHADER_TYPE::CS)
	{
		if (overrideEntryPoint == false)
			desc->entryPoint = L"CS_main";

		desc->target = L"cs_6_5";
		desc->defines.push_back({ L"COMPUTE_SHADER" });
	}
	else if (desc->shaderType == E_SHADER_TYPE::DXR)
	{
		if (overrideEntryPoint == false)
			desc->entryPoint = L"";

		desc->target = L"lib_6_5";
		desc->defines.push_back({ L"DXR_SHADER" });
	}
	else
	{
		BL_ASSERT_MESSAGE(false, "Shadertype not supported yet... Fix!\n");
	}

	m_pBlob = DXILShaderCompiler::GetInstance().Compile(desc);

	BL_ASSERT_MESSAGE(m_pBlob != nullptr, "blob is nullptr when loading shader with path: %S\n", desc->filePath)
}
