#ifndef COPYPERFRAMEMATRICESTASK_H
#define COPYPERFRAMEMATRICESTASK_H

#include "CopyTask.h"

class BaseCamera;
class Transform;

class CopyPerFrameMatricesTask : public CopyTask
{
public:
	CopyPerFrameMatricesTask(
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyPerFrameMatricesTask();


	void Execute();

	void SubmitTransform(Transform* transform);
	void SetCamera(BaseCamera* cam);

private:
	BaseCamera* m_pCameraRef = nullptr;
	std::vector<Transform*> m_TransformsToUpdate;
};

#endif