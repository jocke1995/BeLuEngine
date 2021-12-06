#ifndef D3D12GRAPHICSTEXTURE_H
#define D3D12GRAPHICSTEXTURE_H

#include "../IGraphicsTexture.h"

class D3D12GraphicsTexture : public IGraphicsTexture
{
public:
	D3D12GraphicsTexture();
	virtual ~D3D12GraphicsTexture();


	ID3D12Resource1* GetTempResource() { return m_pResource; }
private:

	ID3D12Resource1* m_pResource = nullptr;
};

#endif
