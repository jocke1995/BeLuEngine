#ifndef DX12TASK_H
#define DX12TASK_H

#include "Core.h"
#include "RenderCore.h"
#include "../Misc/Multithreading/MultiThreadedTask.h"

class IGraphicsContext;
class IGraphicsPipelineState;

// These passes will execute on the graphics commandQueue
enum E_GRAPHICS_PASS_TYPE
{
	COPY_ON_DEMAND,
	BLAS,
	TLAS,
	DEPTH_PRE_PASS,
	DEFERRED_GEOMETRY,
	DEFERRED_LIGHT,
	REFLECTIONS,
	OPACITY,
	WIREFRAME,
	OUTLINE,
	POSTPROCESS_BLOOM,
	POSTPROCESS_TONEMAP,
	IMGUI,
	NR_OF_GRAPHICS_PASSES
};

namespace component
{
	class ModelComponent;
	class TransformComponent;
}

struct RenderComponent
{
public:
	RenderComponent(component::ModelComponent* mc, component::TransformComponent* tc)
		:mc(mc), tc(tc) {};

	component::ModelComponent* mc = nullptr;
	component::TransformComponent* tc = nullptr;
};

class IGraphicsBuffer;
class IGraphicsTexture;

class GraphicsPass : public MultiThreadedTask
{
public:
	GraphicsPass(const std::wstring& passName);
	virtual ~GraphicsPass();

	void AddGraphicsBuffer(std::string id, IGraphicsBuffer* graphicsBuffer);
	void AddGraphicsTexture(std::string id, IGraphicsTexture* graphicsTexture);

	IGraphicsContext* const GetGraphicsContext() const;
protected:
	std::map<std::string, IGraphicsBuffer*> m_GraphicBuffers;
	std::map<std::string, IGraphicsTexture*> m_GraphicTextures;

	IGraphicsContext* m_pGraphicsContext = nullptr;

	std::vector<IGraphicsPipelineState*> m_PipelineStates;
};

#endif