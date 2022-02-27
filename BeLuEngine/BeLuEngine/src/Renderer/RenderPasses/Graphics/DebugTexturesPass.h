#ifndef DEBUGTEXTURESPASS_H
#define DEBUGTEXTURESPASS_H

#include "GraphicsPass.h"

class IGraphicsContext;

class DebugTexturesPass : public GraphicsPass
{
public:
	DebugTexturesPass();
	~DebugTexturesPass();

	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);

private:
	std::vector<RenderComponent> m_RenderComponents;

	void drawRenderComponent(
		component::ModelComponent* mc,
		component::TransformComponent* tc,
		IGraphicsContext* graphicsContext);
};

#endif