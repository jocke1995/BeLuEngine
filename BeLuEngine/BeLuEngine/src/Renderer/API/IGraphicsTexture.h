#ifndef IGRAPHICSTEXTURE_H
#define IGRAPHICSTEXTURE_H

enum F_TEXTURE_USAGE
{
	ShaderResource	 = BIT(0),
	RenderTarget	 = BIT(1),
	UnorderedAccess  = BIT(2),
	DepthStencil	 = BIT(3)
};

class IGraphicsTexture
{
public:
	virtual ~IGraphicsTexture();

	static IGraphicsTexture* Create();

	virtual bool LoadTextureDDS(const std::wstring& filePath) = 0;
	virtual bool CreateTexture2D(unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat, unsigned int textureUsage /* F_TEXTURE_USAGE */, const std::wstring name, D3D12_RESOURCE_STATES startStateTemp) = 0;

	virtual unsigned int GetShaderResourceHeapIndex() const = 0;
	virtual unsigned int GetRenderTargetHeapIndex() const = 0;
	virtual unsigned int GetUnorderedAccessIndex() const = 0;
	virtual unsigned int GetDepthStencilIndex() const = 0;

	virtual unsigned __int64 GetSize() const = 0;

	const std::wstring GetPath() const { return m_Path; }
protected:
	std::wstring m_Path = L"";
};

#endif