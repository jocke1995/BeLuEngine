#include "stdafx.h"
#include "RenderTarget.h"

#include "DescriptorHeap.h"
#include "Resource.h"
#include "RenderView.h"

RenderTarget::RenderTarget(
	ID3D12Device5* device,
	unsigned int width, unsigned int height,
	DescriptorHeap* descriptorHeap_RTV)
{
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	
	// View Desc
	D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
	viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	
	// Create m_Resources and RTVs
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		Resource* resource = new Resource(
			device,
			&resourceDesc,
			nullptr,
			L"RenderTarget_DEFAULT_RESOURCE",
			D3D12_RESOURCE_STATE_GENERIC_READ);
	
		m_Resources.push_back(resource);

		unsigned int dhIndex = descriptorHeap_RTV->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = descriptorHeap_RTV->GetCPUHeapAt(dhIndex);
		device->CreateRenderTargetView(resource->GetID3D12Resource1(), &viewDesc, cdh);
	}
	
	m_pRenderView = new RenderView(width, height);
}

RenderTarget::RenderTarget(unsigned int width, unsigned int height)
{
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_Resources.push_back(new Resource());
	}

	m_pRenderView = new RenderView(width, height);
}

RenderTarget::~RenderTarget()
{
	for (Resource* resource : m_Resources)
	{
		delete resource;
	}

	delete m_pRenderView;
}

Resource* RenderTarget::GetResource(unsigned int index) const
{
	return m_Resources[index];
}

RenderView* RenderTarget::GetRenderView() const
{
	return m_pRenderView;
}
