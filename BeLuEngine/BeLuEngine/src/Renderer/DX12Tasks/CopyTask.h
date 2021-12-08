#ifndef COPYTASK_H
#define COPYTASK_H

#include "DX12Task.h"

class Resource;
class IGraphicsBuffer;
class IGraphicsTexture;

struct GraphicsBufferUploadParams
{
	IGraphicsBuffer* graphicsBuffer;
	const void* data;
};

struct GraphicsTextureUploadParams
{
	IGraphicsTexture* graphicsTexture;
	const void* data;
};

class CopyTask : public DX12Task
{
public:
	CopyTask(
		ID3D12Device5* device,
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyTask();

	void CopyTask::ClearSpecific(const Resource* uploadResource);

	void SubmitBuffer(IGraphicsBuffer* graphicsBuffer, const void* data);
	void SubmitTexture(IGraphicsTexture* graphicsTexture, const void* data);

	virtual void Clear() = 0;

protected:
	std::vector<GraphicsBufferUploadParams> m_GraphicBuffersToUpload;
	std::vector<GraphicsTextureUploadParams> m_GraphicTexturesToUpload;

	void copyResource(
		ID3D12GraphicsCommandList5* commandList,
		Resource* uploadResource, Resource* defaultResource,
		const void* data);
};
#endif