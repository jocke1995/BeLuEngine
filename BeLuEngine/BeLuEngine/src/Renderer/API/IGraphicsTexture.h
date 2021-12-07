#ifndef IGRAPHICSTEXTURE_H
#define IGRAPHICSTEXTURE_H

enum F_TEXTURE_USAGE
{
	Normal = BIT(0),
	RenderTarget = BIT(1),
	UnorderedAccess = BIT(2),
	Depth = BIT(3)
};

class IGraphicsTexture
{
public:
	virtual ~IGraphicsTexture();

	static IGraphicsTexture* Create();

	virtual bool CreateTexture2D(const std::wstring& filePath, DXGI_FORMAT dxgiFormat, F_TEXTURE_USAGE textureUsage) = 0;
protected:
};

#endif