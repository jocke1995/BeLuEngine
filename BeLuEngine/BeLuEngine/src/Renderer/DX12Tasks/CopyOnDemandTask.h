#ifndef COPYONDEMANDTASK_H
#define COPYONDEMANDTASK_H

#include "CopyTask.h"

class IGraphicsBuffer;
class IGraphicsTexture;

class CopyOnDemandTask : public CopyTask
{
public:
	CopyOnDemandTask(
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyOnDemandTask();

	void SubmitBuffer(IGraphicsBuffer* graphicsBuffer, const void* data);
	void SubmitTexture(IGraphicsTexture* graphicsTexture);

	void Execute() override final;
	void Clear();
private:
	std::vector<std::pair<IGraphicsBuffer*, const void*>> m_GraphicBuffersToUpload;
	std::vector<IGraphicsTexture*> m_GraphicTexturesToUpload;

	void CopyTexture(ID3D12GraphicsCommandList* cl, IGraphicsTexture* graphicsTexture);
	void CopyBuffer(ID3D12GraphicsCommandList* cl, std::pair<IGraphicsBuffer*, const void*>* graphicsBuffer_data);
};

#endif
