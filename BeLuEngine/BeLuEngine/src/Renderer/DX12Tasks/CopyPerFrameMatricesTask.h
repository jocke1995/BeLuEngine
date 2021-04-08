#ifndef COPYPERFRAMEMATRICESTASK_H
#define COPYPERFRAMEMATRICESTASK_H

#include "CopyTask.h"

class BaseCamera;

class CopyPerFrameMatricesTask : public CopyTask
{
public:
	CopyPerFrameMatricesTask(
		ID3D12Device5* device,
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyPerFrameMatricesTask();

	void SetCamera(BaseCamera* cam);
	// Removal
	void Clear() override;

	void Execute();

private:
	BaseCamera* m_pCameraRef = nullptr;
};

#endif