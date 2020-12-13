#ifndef SHADOWINFO_H
#define SHADOWINFO_H

class DepthStencilView;
class ShaderResourceView;
class RenderView;
class DescriptorHeap;
class Resource;

#include "../ECS/Components/Lights/Light.h"

class ShadowInfo
{
public:
	ShadowInfo(
		LIGHT_TYPE lightType,
		SHADOW_RESOLUTION shadowResolution,
		ID3D12Device5* device,
		DescriptorHeap* dh_DSV,
		DescriptorHeap* dh_SRV);

	virtual ~ShadowInfo();

	bool operator == (const ShadowInfo& other);
	bool operator != (const ShadowInfo& other);

	unsigned int GetId() const;
	SHADOW_RESOLUTION GetShadowResolution() const;
	Resource* GetResource() const;
	DepthStencilView* GetDSV() const;
	ShaderResourceView* GetSRV() const;
	RenderView* GetRenderView() const;

private:
	inline static unsigned int s_IdCounter = 0;
	unsigned int m_Id = 0;
	SHADOW_RESOLUTION m_ShadowResolution = SHADOW_RESOLUTION::UNDEFINED;

	Resource* m_pResource = nullptr;
	DepthStencilView* m_pDSV = nullptr;
	ShaderResourceView* m_pSRV = nullptr;
	RenderView* m_pRenderView = nullptr;
	
	void createResource(ID3D12Device5* device, unsigned int width, unsigned int height);
	void createDSV(ID3D12Device5* device, DescriptorHeap* dh_DSV);
	void createSRV(ID3D12Device5* device, DescriptorHeap* dh_SRV);
};
#endif
