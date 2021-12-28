#ifndef D3D12GRAPHICSTEXTURE_H
#define D3D12GRAPHICSTEXTURE_H

#include "../IGraphicsTexture.h"

class D3D12DescriptorHeap;

class D3D12GraphicsTexture : public IGraphicsTexture
{
public:
	D3D12GraphicsTexture();
	virtual ~D3D12GraphicsTexture();

	bool LoadTextureDDS(const std::wstring& filePath) override;
	bool CreateTexture2D(unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat, unsigned int textureUsage /* F_TEXTURE_USAGE */, const std::wstring name, D3D12_RESOURCE_STATES startStateTemp, unsigned int mipLevels = 1) override;

	virtual unsigned int GetShaderResourceHeapIndex(unsigned int mipLevel = 0) const override;
	virtual unsigned int GetUnorderedAccessIndex(unsigned int mipLevel = 0) const override;
	virtual unsigned int GetRenderTargetHeapIndex() const override;
	virtual unsigned int GetDepthStencilIndex() const override;

	virtual unsigned __int64 GetSize() const override;

	std::vector<D3D12_SUBRESOURCE_DATA>* GetTempSubresources() { return &m_Subresources; }
private:
	friend class D3D12GraphicsContext;

	friend class TonemapComputeTask;	// Temporary

	ID3D12Resource1* m_pResource = nullptr;
	unsigned int m_ShaderResourceDescriptorHeapIndices[g_MAX_TEXTURE_MIPS]	= { UINT32_MAX };
	unsigned int m_UnorderedAccessDescriptorHeapIndices[g_MAX_TEXTURE_MIPS] = { UINT32_MAX };
	unsigned int m_RenderTargetDescriptorHeapIndex		= -1;
	unsigned int m_DepthStencilDescriptorHeapIndex		= -1;

	D3D12DescriptorHeap* m_CPUDescriptorHeap = nullptr;

	unsigned __int64 m_Size = 0;
	unsigned char* m_pTextureData = nullptr;

	unsigned int m_NumMipLevels = 0;

	std::vector<D3D12_SUBRESOURCE_DATA> m_Subresources;
};

#endif
