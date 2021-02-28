#ifndef SHADER_H
#define SHADER_H

class Shader
{
public:
	Shader(LPCTSTR path, E_SHADER_TYPE type);
	virtual ~Shader();

	ID3DBlob* GetBlob() const;

private:
	ID3DBlob* m_pBlob;
	E_SHADER_TYPE m_Type;
	LPCTSTR m_Path;	// Ex: vertexShader1

	void compileShader();
};

#endif