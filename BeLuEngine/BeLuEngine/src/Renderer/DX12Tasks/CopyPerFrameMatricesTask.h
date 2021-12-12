#ifndef COPYPERFRAMEMATRICESTASK_H
#define COPYPERFRAMEMATRICESTASK_H

#include "GraphicsPass.h"

class BaseCamera;
class Transform;

class CopyPerFrameMatricesTask : public GraphicsPass
{
public:
	CopyPerFrameMatricesTask();
	virtual ~CopyPerFrameMatricesTask();

	void Execute();

	void SubmitTransform(Transform* transform);
	void SetCamera(BaseCamera* baseCamera) { m_pCamera = baseCamera; }

private:
	BaseCamera* m_pCamera = nullptr;
	std::vector<Transform*> m_TransformsToUpdate;
};

#endif