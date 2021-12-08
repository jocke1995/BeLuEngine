#ifndef COPYTASK_H
#define COPYTASK_H

#include "DX12Task.h"

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

struct ID3D12GraphicsCommandList;

class CopyTask : public DX12Task
{
public:
	CopyTask(
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyTask();

	void SubmitBuffer(IGraphicsBuffer* graphicsBuffer, const void* data);
	void SubmitTexture(IGraphicsTexture* graphicsTexture, const void* data);

	void Clear();

protected:
	std::vector<GraphicsBufferUploadParams> m_GraphicBuffersToUpload;
	std::vector<GraphicsTextureUploadParams> m_GraphicTexturesToUpload;

	void CopyTexture(ID3D12GraphicsCommandList* cl, GraphicsTextureUploadParams* uploadParams);
	void CopyBuffer(ID3D12GraphicsCommandList* cl, GraphicsBufferUploadParams* uploadParams);
};
#endif