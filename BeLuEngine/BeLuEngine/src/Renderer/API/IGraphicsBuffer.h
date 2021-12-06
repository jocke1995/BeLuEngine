#ifndef IGRAPHICSBUFFER_H
#define IGRAPHICSBUFFER_H

enum class E_GRAPHICSBUFFER_TYPE
{
	ConstantBuffer,
	RawBuffer,
	None,
	NUM_PARAMS
};

enum class E_GRAPHICSBUFFER_UPLOADFREQUENCY
{
	Static,
	Dynamic,
	None,
	NUM_PARAMS
};

class IGraphicsBuffer
{
public:
	virtual ~IGraphicsBuffer();

	static IGraphicsBuffer* Create(E_GRAPHICSBUFFER_TYPE type, E_GRAPHICSBUFFER_UPLOADFREQUENCY uploadFrequency, unsigned int size, std::wstring name);

	virtual unsigned int GetSize() const = 0;

protected:
	unsigned int m_Size = 0;
};

#endif