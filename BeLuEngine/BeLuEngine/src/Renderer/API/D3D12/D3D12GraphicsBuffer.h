#ifndef D3D12GRAPHICSBUFFER_H
#define D3D12GRAPHICSBUFFER_H

#include "../IGraphicsBuffer.h"

class D3D12GraphicsBuffer : public IGraphicsBuffer
{
public:
	D3D12GraphicsBuffer(E_GRAPHICSBUFFER_TYPE type, unsigned int sizeOfSingleItem, unsigned int numItems, DXGI_FORMAT format, std::wstring name);
	virtual ~D3D12GraphicsBuffer();

	unsigned int GetConstantBufferDescriptorIndex() const override;
	unsigned int GetShaderResourceHeapIndex() const override;

	unsigned int GetSize() const override { return m_Size; }
private:
	friend class D3D12GraphicsContext;

	TODO("These are temporary until dxr is abstracted away");
	friend class BottomLevelAccelerationStructure;
	friend class TopLevelAccelerationStructure;
	friend class DXRReflectionTask;
	friend class CopyPerFrameMatricesTask;

	ID3D12Resource1* m_pResource = nullptr;
	unsigned int m_ConstantBufferDescriptorHeapIndex = -1;
	unsigned int m_ShaderResourceDescriptorHeapIndex = -1;

	E_GRAPHICSBUFFER_TYPE m_BufferType = E_GRAPHICSBUFFER_TYPE::None;
};

#endif
