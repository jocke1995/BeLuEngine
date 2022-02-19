#ifndef D3D12GRAPHICSBUFFER_H
#define D3D12GRAPHICSBUFFER_H

#include "../Interface/IGraphicsBuffer.h"

class D3D12GlobalStateTracker;

class D3D12GraphicsBuffer : public IGraphicsBuffer
{
public:
	D3D12GraphicsBuffer(E_GRAPHICSBUFFER_TYPE type, unsigned int sizeOfSingleItem, unsigned int numItems, BL_FORMAT format, std::wstring name);
	virtual ~D3D12GraphicsBuffer();

	virtual bool SetData(unsigned int size, const void* data) override;

	unsigned int GetConstantBufferDescriptorIndex() const override;
	unsigned int GetShaderResourceHeapIndex() const override;

	unsigned int GetSize() const override { return m_Size; }
private:
	friend class D3D12GraphicsContext;
	friend class D3D12TopLevelAS;
	friend class D3D12ShaderBindingTable;
	friend class D3D12BottomLevelAS;

	ID3D12Resource1* m_pResource = nullptr;
	unsigned int m_ConstantBufferDescriptorHeapIndex = -1;
	unsigned int m_ShaderResourceDescriptorHeapIndex = -1;

	E_GRAPHICSBUFFER_TYPE m_BufferType = E_GRAPHICSBUFFER_TYPE::None;

	// Resource Tracker
	D3D12GlobalStateTracker* m_GlobalStateTracker = nullptr;
};

#endif
