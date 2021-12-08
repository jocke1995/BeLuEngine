#include "stdafx.h"
#include "DX12Task.h"

#include "../Misc/Log.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"

// PIX Events
#include "WinPixEventRuntime/pix3.h"

#include "../API/IGraphicsBuffer.h"
#include "../API/IGraphicsTexture.h"

ScopedPIXEvent::ScopedPIXEvent(const char* nameOfTask, ID3D12GraphicsCommandList* cl)
{
	BL_ASSERT(cl);

	m_pCommandList = cl;
	UINT64 col = 0;
	PIXBeginEvent(m_pCommandList, col, nameOfTask);
}

ScopedPIXEvent::~ScopedPIXEvent()
{
	PIXEndEvent(m_pCommandList);
}

// DX12 TASK
DX12Task::DX12Task(
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:MultiThreadedTask(FLAG_THREAD)
{
	m_pCommandInterface = new CommandInterface(interfaceType, clName);
}

DX12Task::~DX12Task()
{
	delete m_pCommandInterface;
}

void DX12Task::SetBackBufferIndex(int backBufferIndex)
{
	m_BackBufferIndex = backBufferIndex;
}

void DX12Task::SetCommandInterfaceIndex(int index)
{
	BL_ASSERT(index > 0);

	m_CommandInterfaceIndex = index;
}

void DX12Task::AddGraphicsBuffer(std::string id, IGraphicsBuffer* graphicBuffer)
{
	BL_ASSERT_MESSAGE(m_GraphicBuffers[id] == nullptr, "Trying to add graphicBuffer with name: \'%s\' that already exists.\n, id");
	m_GraphicBuffers[id] = graphicBuffer;
}

void DX12Task::AddGraphicsTexture(std::string id, IGraphicsTexture* graphicTexture)
{
	BL_ASSERT_MESSAGE(m_GraphicTextures[id] == nullptr, "Trying to add graphicTexture with name: \'%s\' that already exists.\n, id");
	m_GraphicTextures[id] = graphicTexture;
}

CommandInterface* const DX12Task::GetCommandInterface() const
{
	BL_ASSERT(m_pCommandInterface);
	return m_pCommandInterface;
}
