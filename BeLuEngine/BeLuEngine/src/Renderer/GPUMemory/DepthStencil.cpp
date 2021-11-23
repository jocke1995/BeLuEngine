#include "stdafx.h"
#include "DepthStencil.h"

#include "Resource.h"
#include "DepthStencilView.h"
#include "ShaderResourceView.h"
#include "../DescriptorHeap.h"

DepthStencil::DepthStencil(
    ID3D12Device5* device,
	unsigned int width,
	unsigned int height,
    std::wstring resourceName,
    D3D12_DEPTH_STENCIL_VIEW_DESC* dsvDesc,
    DescriptorHeap* descriptorHeap_DSV,
    DescriptorHeap* descriptorHeap_CBV_UAV_SRV)
{
    createResource(device, width, height, resourceName, dsvDesc->Format);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	m_pDSV = new DepthStencilView(device, descriptorHeap_DSV, dsvDesc, m_pDefaultResource);
	m_pSRV = new ShaderResourceView(device, descriptorHeap_CBV_UAV_SRV, &srvDesc, m_pDefaultResource);

    m_Id = s_DsCounter++;
}

bool DepthStencil::operator==(const DepthStencil& other)
{
    return m_Id == other.m_Id;
}

bool DepthStencil::operator!=(const DepthStencil& other)
{
	return !(operator==(other));
}

DepthStencil::~DepthStencil()
{
    delete m_pDefaultResource;
    delete m_pDSV;
	delete m_pSRV;
}

const Resource* DepthStencil::GetDefaultResource() const
{
    return m_pDefaultResource;
}

const DepthStencilView* DepthStencil::GetDSV() const
{
    return m_pDSV;
}

ShaderResourceView* const DepthStencil::GetSRV() const
{
	return m_pSRV;
}

void DepthStencil::createResource(ID3D12Device5* device, unsigned int width, unsigned int height, std::wstring resourceName, DXGI_FORMAT format)
{
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Format = format;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil.Depth = 1;
	clearValue.DepthStencil.Stencil = 0;

	m_pDefaultResource = new Resource(
		device,
		&resourceDesc,
		&clearValue,
		resourceName,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
