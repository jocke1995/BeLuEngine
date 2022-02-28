#ifndef DEBUGTEXTURESPASS_H
#define DEBUGTEXTURESPASS_H

#include "GraphicsPass.h"

class IGraphicsContext;
class Mesh;

enum E_DEBUG_TEXTURE_VISUALIZATION
{
	E_DEBUG_TEXTURE_VISUALIZATION_ALBEDO,
	E_DEBUG_TEXTURE_VISUALIZATION_NORMAL,
	E_DEBUG_TEXTURE_VISUALIZATION_ROUGHNESS,
	E_DEBUG_TEXTURE_VISUALIZATION_METALLIC,
	E_DEBUG_TEXTURE_VISUALIZATION_EMISSIVE,
	E_DEBUG_TEXTURE_VISUALIZATION_REFLECTION,
	E_DEBUG_TEXTURE_VISUALIZATION_DEPTH,
	E_DEBUG_TEXTURE_VISUALIZATION_STENCIL,
	E_DEBUG_TEXTURE_VISUALIZATION_FINALCOLOR,
	NUM_TEXTURES_TO_VISUALIZE
};

class DebugTexturesPass : public GraphicsPass
{
public:
	DebugTexturesPass(Mesh* fullScreenQuad);
	~DebugTexturesPass();

	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);

	void AdvanceTextureToVisualize();

private:
	std::vector<RenderComponent> m_RenderComponents;
	Mesh* m_pFullScreenQuad = nullptr;

	unsigned int m_CurrentTextureToVisualize = E_DEBUG_TEXTURE_VISUALIZATION_FINALCOLOR;

	void drawRenderComponent(component::ModelComponent* mc, component::TransformComponent* tc, IGraphicsContext* graphicsContext);
	void createStatePermutation(const E_DEBUG_TEXTURE_VISUALIZATION psoIndex, const std::vector<LPCWSTR> defines);
};

#endif