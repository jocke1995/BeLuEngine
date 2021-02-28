#ifndef SHADERRESOURCEVIEW_H
#define SHADERRESOURCEVIEW_H

#include "View.h"

class ShaderResourceView : public View
{
public:
	ShaderResourceView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
		D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc,
		const Resource* const resource);

	// Default SRV-Desc
	ShaderResourceView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
		const Resource* const resource);

	virtual ~ShaderResourceView();


private:
	void createShaderResourceView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
		D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc);
};

#endif
