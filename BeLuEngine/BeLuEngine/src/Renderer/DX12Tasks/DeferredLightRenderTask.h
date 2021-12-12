#ifndef DEFERREDLIGHTASK_H
#define DEFERREDLIGHTASK_H

#include "GraphicsPass.h"

class Mesh;

class DeferredLightRenderTask : public GraphicsPass
{
public:
	DeferredLightRenderTask(Mesh* fullscreenQuad);
	~DeferredLightRenderTask();

	void Execute() override final;

private:
	Mesh* m_pFullScreenQuadMesh = nullptr;

};

#endif