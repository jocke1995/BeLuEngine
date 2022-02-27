#ifndef SHADER_H
#define SHADER_H

#include "RenderCore.h"

struct IDxcBlob;

struct DXILCompilationDesc;

class Shader
{
public:
	Shader(DXILCompilationDesc* desc);
	virtual ~Shader();

	IDxcBlob* GetBlob() const;

private:
	IDxcBlob* m_pBlob;

	void compileShader(DXILCompilationDesc* desc);
};

#endif