#include "stdafx.h"
#include "ShaderResourceView.h"

#include "../DescriptorHeap.h"
#include "Resource.h"

ShaderResourceView::ShaderResourceView(
	ID3D12Device5* device,
	DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
	D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc,
	const Resource* const resource)
	:View(descriptorHeap_CBV_UAV_SRV, resource)
{
	createShaderResourceView(device, descriptorHeap_CBV_UAV_SRV, srvDesc);
}

ShaderResourceView::ShaderResourceView(ID3D12Device5* device, DescriptorHeap* descriptorHeap_CBV_UAV_SRV, const Resource* const resource)
	: View(descriptorHeap_CBV_UAV_SRV, resource)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	createShaderResourceView(device, descriptorHeap_CBV_UAV_SRV, &srvDesc);
}

ShaderResourceView::~ShaderResourceView()
{
	
}

void ShaderResourceView::createShaderResourceView(
	ID3D12Device5* device,
	DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
	D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = descriptorHeap_CBV_UAV_SRV->GetCPUHeapAt(m_DescriptorHeapIndex);
	device->CreateShaderResourceView(m_pResource->GetID3D12Resource1(), srvDesc, cdh);
}
