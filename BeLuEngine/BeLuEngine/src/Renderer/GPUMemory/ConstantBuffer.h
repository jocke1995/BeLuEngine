#ifndef CONSTANTBUFFER_H
#define CONSTANTBUFFER_H

class Resource;
class DescriptorHeap;
class ConstantBufferView;

static unsigned int s_CbCounter = 0;
class ConstantBuffer
{
public:
	ConstantBuffer(ID3D12Device5* device,
		unsigned int entrySize,
		std::wstring resourceName,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV);

	virtual ~ConstantBuffer();

	bool operator == (const ConstantBuffer& other);
	bool operator != (const ConstantBuffer& other);

	Resource* GetUploadResource() const;
	Resource* GetDefaultResource() const;

	const ConstantBufferView* const GetCBV() const;
	
private:
	Resource* m_pUploadResource = nullptr;
	Resource* m_pDefaultResource = nullptr;
	ConstantBufferView* m_pCBV = nullptr;

	unsigned int m_Id = 0;

};

#endif
