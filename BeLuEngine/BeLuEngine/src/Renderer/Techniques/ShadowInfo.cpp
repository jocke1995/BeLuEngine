#include "stdafx.h"
#include "ShadowInfo.h"

#include "../GPUMemory/GPUMemory.h"
#include "../DescriptorHeap.h"
#include "../RenderView.h"

ShadowInfo::ShadowInfo(
	E_LIGHT_TYPE lightType,
	E_SHADOW_RESOLUTION shadowResolution,
	ID3D12Device5* device,
	DescriptorHeap* dh_DSV,
	DescriptorHeap* dh_SRV)
{
	unsigned int depthTextureWidth = 0;
	unsigned int depthTextureHeight = 0;
	switch (lightType)
	{
	case E_LIGHT_TYPE::DIRECTIONAL_LIGHT:
		switch (shadowResolution)
		{
		case E_SHADOW_RESOLUTION::LOW:
			depthTextureWidth = 1024;
			depthTextureHeight = 1024;
			break;
		case E_SHADOW_RESOLUTION::MEDIUM:
			depthTextureWidth = 2048;
			depthTextureHeight = 2048;
			break;
		case E_SHADOW_RESOLUTION::HIGH:
			depthTextureWidth = 4096;
			depthTextureHeight = 4096;
			break;
		}
		break;
	case E_LIGHT_TYPE::POINT_LIGHT:
		switch (shadowResolution)
		{
		case E_SHADOW_RESOLUTION::LOW:
			depthTextureWidth = 512;
			depthTextureHeight = 512;
			break;
		case E_SHADOW_RESOLUTION::MEDIUM:
			depthTextureWidth = 1024;
			depthTextureHeight = 1024;
			break;
		case E_SHADOW_RESOLUTION::HIGH:
			depthTextureWidth = 2048;
			depthTextureHeight = 2048;
			break;
		}
		break;
	case E_LIGHT_TYPE::SPOT_LIGHT:
		switch (shadowResolution)
		{
		case E_SHADOW_RESOLUTION::LOW:
			depthTextureWidth = 1024;
			depthTextureHeight = 1024;
			break;
		case E_SHADOW_RESOLUTION::MEDIUM:
			depthTextureWidth = 2048;
			depthTextureHeight = 2048;
			break;
		case E_SHADOW_RESOLUTION::HIGH:
			depthTextureWidth = 4096;
			depthTextureHeight = 4096;
			break;
		}
		break;
	}

	m_Id = s_IdCounter++;
	m_ShadowResolution = shadowResolution;

	createResource(device, depthTextureWidth, depthTextureHeight);

	createDSV(device, dh_DSV);
	createSRV(device, dh_SRV);

	m_pRenderView = new RenderView(depthTextureWidth, depthTextureHeight);

}

ShadowInfo::~ShadowInfo()
{
	delete m_pResource;

	delete m_pDSV;
	delete m_pSRV;

	delete m_pRenderView;
}

bool ShadowInfo::operator==(const ShadowInfo& other)
{
	return m_Id == other.m_Id;
}

bool ShadowInfo::operator!=(const ShadowInfo& other)
{
	return !(operator==(other));
}

unsigned int ShadowInfo::GetId() const
{
	return m_Id;
}

E_SHADOW_RESOLUTION ShadowInfo::GetShadowResolution() const
{
	return m_ShadowResolution;
}

Resource* ShadowInfo::GetResource() const
{
	return m_pResource;
}

DepthStencilView* ShadowInfo::GetDSV() const
{
	return m_pDSV;
}

ShaderResourceView* ShadowInfo::GetSRV() const
{
	return m_pSRV;
}

RenderView* ShadowInfo::GetRenderView() const
{
	return m_pRenderView;
}

void ShadowInfo::createResource(ID3D12Device5* device, unsigned int width, unsigned int height)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Alignment = 0;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	std::wstring resourceName = L"ShadowMap" + std::to_wstring(m_Id) + L"_DEFAULT_RESOURCE";
	m_pResource = new Resource(
		device,
		&desc,
		&clearValue,
		resourceName,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void ShadowInfo::createDSV(ID3D12Device5* device, DescriptorHeap* dh_DSV)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_pDSV = new DepthStencilView(
		device,
		dh_DSV,
		&dsvDesc,
		m_pResource);
}

void ShadowInfo::createSRV(ID3D12Device5* device, DescriptorHeap* dh_SRV)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = 1;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.ResourceMinLODClamp = 0.0f;
	srvd.Texture2D.PlaneSlice = 0;

	m_pSRV = new ShaderResourceView(
		device,
		dh_SRV,
		&srvd,
		m_pResource);
}
