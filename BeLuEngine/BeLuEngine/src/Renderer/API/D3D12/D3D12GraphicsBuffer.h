#ifndef D3D12GRAPHICSBUFFER_H
#define D3D12GRAPHICSBUFFER_H

#include "../IGraphicsBuffer.h"

class D3D12GraphicsBuffer : public IGraphicsBuffer
{
public:
	D3D12GraphicsBuffer(E_GRAPHICSBUFFER_TYPE type, E_GRAPHICSBUFFER_UPLOADFREQUENCY uploadFrequency, unsigned int size, std::wstring name);
	virtual ~D3D12GraphicsBuffer();

	unsigned int GetSize() const override { return m_Size; }

	unsigned int GetConstantBufferDescriptorIndex() const;
	unsigned int GetRawBufferDescriptorIndex() const;

private:
	E_GRAPHICSBUFFER_TYPE m_BufferType = E_GRAPHICSBUFFER_TYPE::None;

	ID3D12Resource1* m_pResource = nullptr;

	// DescriptorBindings
	unsigned int m_ConstantBufferSlot = 0;
	unsigned int m_RawBufferSlot = 0;
};

#endif
