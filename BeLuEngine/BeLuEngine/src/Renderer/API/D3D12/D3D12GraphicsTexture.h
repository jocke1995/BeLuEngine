#ifndef D3D12GRAPHICSTEXTURE_H
#define D3D12GRAPHICSTEXTURE_H

#include "../IGraphicsTexture.h"

class D3D12GraphicsTexture : public IGraphicsTexture
{
public:
	D3D12GraphicsTexture();
	virtual ~D3D12GraphicsTexture();

	bool CreateTexture2D(const std::wstring& filePath, DXGI_FORMAT dxgiFormat, F_TEXTURE_USAGE textureUsage) override;

	ID3D12Resource1* GetTempResource() { return m_pResource; }
private:

	ID3D12Resource1* m_pResource = nullptr;
};

#endif
