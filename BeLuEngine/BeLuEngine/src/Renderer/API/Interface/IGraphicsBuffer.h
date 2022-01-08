#ifndef IGRAPHICSBUFFER_H
#define IGRAPHICSBUFFER_H

enum class E_GRAPHICSBUFFER_TYPE
{
	ConstantBuffer,
	RawBuffer,
	VertexBuffer,
	IndexBuffer,
	CPUBuffer,
	UnorderedAccessBuffer,
	RayTracingBuffer,
	None,
	NUM_PARAMS
};

class IGraphicsBuffer
{
public:
	virtual ~IGraphicsBuffer();

	static IGraphicsBuffer* Create(E_GRAPHICSBUFFER_TYPE type, unsigned int sizeOfSingleItem, unsigned int numItems, DXGI_FORMAT format, std::wstring name);

	virtual unsigned int GetConstantBufferDescriptorIndex() const = 0;
	virtual unsigned int GetShaderResourceHeapIndex() const = 0;

	virtual unsigned int GetSize() const = 0;

protected:
	unsigned int m_Size = 0;

#ifdef DEBUG
	std::wstring m_DebugName = L"";
#endif
};

#endif