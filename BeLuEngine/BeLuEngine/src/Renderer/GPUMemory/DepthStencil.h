#ifndef DEPTHSTENCIL_H
#define DEPTHSTENCIL_H

class Resource;
class DescriptorHeap;
class DepthStencilView;
class ShaderResourceView;

static unsigned int s_DsCounter = 0;
class DepthStencil
{
public:
	DepthStencil(
		ID3D12Device5* device,
		unsigned int width,
		unsigned int height,
		std::wstring resourceName,
		D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc,
		DescriptorHeap* descriptorHeap_DSV,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV);

	virtual ~DepthStencil();

	bool operator == (const DepthStencil& other);
	bool operator != (const DepthStencil& other);

	const Resource* const GetDefaultResource() const;
	const DepthStencilView* const GetDSV() const;
	ShaderResourceView* const GetSRV() const;

private:
	Resource* m_pDefaultResource = nullptr;
	DepthStencilView* m_pDSV = nullptr;
	ShaderResourceView* m_pSRV = nullptr;

	unsigned int m_Id = 0;
	void createResource(
		ID3D12Device5* device,
		unsigned int width, unsigned int height,
		std::wstring resourceName,
		DXGI_FORMAT format);
};

#endif
