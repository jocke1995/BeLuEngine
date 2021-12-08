#ifndef COPYPERFRAMEMATRICESTASK_H
#define COPYPERFRAMEMATRICESTASK_H

#include "CopyTask.h"

class BaseCamera;

class CopyPerFrameMatricesTask : public CopyTask
{
public:
	CopyPerFrameMatricesTask(
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyPerFrameMatricesTask();

	void SetCamera(BaseCamera* cam);

	void Execute();

private:
	BaseCamera* m_pCameraRef = nullptr;
};

#endif