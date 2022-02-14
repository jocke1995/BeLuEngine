#ifndef SKYBOXPASS_H
#define SKYBOXPASS_H

#include "GraphicsPass.h"

class BaseCamera;

namespace component
{
	class SkyboxComponent;
}

class SkyboxPass : public GraphicsPass
{
public:
	SkyboxPass();
	~SkyboxPass();

	void Execute() override final;

	void SetSkybox(component::SkyboxComponent* skyboxComponent);
	void SetCamera(BaseCamera* camera);
private:
	component::SkyboxComponent* m_pSkyboxComponent = nullptr;
	BaseCamera* m_pBaseCamera = nullptr;
};

#endif