#ifndef CONSTANTBUFFER_H
#define CONSTANTBUFFER_H

#include "Core.h"

class Resource;
class DescriptorHeap;

// DX12 Forward Declarations
struct ID3D12Device5;

class ConstantBuffer
{
public:
	ConstantBuffer(	ID3D12Device5* device,
					unsigned int entrySize,
					std::wstring resourceName,
					unsigned int descriptorHeapIndex);
	virtual ~ConstantBuffer();

	
	Resource* GetUploadResource() const;
	unsigned int GetDescriptorHeapIndex() const;

protected:
	unsigned int m_DescriptorHeapIndex = -1;

	// will be created with different m_Resources depending on the constantBuffer type
	Resource* m_pUploadResource = nullptr;
	virtual void CreateConstantBufferView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV) = 0;
};

#endif
