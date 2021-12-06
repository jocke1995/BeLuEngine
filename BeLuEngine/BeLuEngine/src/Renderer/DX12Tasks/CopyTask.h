#ifndef COPYTASK_H
#define COPYTASK_H

#include "DX12Task.h"

class Resource;
class IGraphicsBuffer;

struct GraphicsUploadParams
{
	IGraphicsBuffer* graphicsBuffer;
	void* data;
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

	// tuple(Upload, Default, Data)
	void Submit(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data);
	void SubmitBuffer(IGraphicsBuffer* graphicsBuffer, void* data);

	virtual void Clear() = 0;

protected:
	std::vector<std::tuple<Resource*, Resource*, const void*>> m_UploadDefaultData;
	std::vector<GraphicsUploadParams> m_GraphicBuffersToUpload;

	void copyResource(
		ID3D12GraphicsCommandList5* commandList,
		Resource* uploadResource, Resource* defaultResource,
		const void* data);
};
#endif