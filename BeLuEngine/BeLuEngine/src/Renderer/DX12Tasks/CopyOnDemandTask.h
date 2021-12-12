#ifndef COPYONDEMANDTASK_H
#define COPYONDEMANDTASK_H

#include "GraphicsPass.h"

class IGraphicsBuffer;
class IGraphicsTexture;

class CopyOnDemandTask : public GraphicsPass
{
public:
	CopyOnDemandTask();
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
