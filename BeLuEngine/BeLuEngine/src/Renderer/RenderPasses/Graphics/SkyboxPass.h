#ifndef SKYBOXPASS_H
#define SKYBOXPASS_H

#include "GraphicsPass.h"

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
private:
	component::SkyboxComponent* m_pSkyboxComponent = nullptr;
};

#endif