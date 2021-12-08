#ifndef GRAPHICSSTATE_H
#define GRAPHICSSTATE_H

#include "PipelineState.h"
class Shader;

// DX12 Forward Declarations
struct D3D12_GRAPHICS_PIPELINE_STATE_DESC;

class GraphicsState : public PipelineState
{
public:
	GraphicsState(
		const std::wstring& VSName, const std::wstring& PSName,
		D3D12_GRAPHICS_PIPELINE_STATE_DESC* gpsd,
		const std::wstring& psoName);

	virtual ~GraphicsState();

	const D3D12_GRAPHICS_PIPELINE_STATE_DESC* GetGpsd() const;
	Shader* GetShader(E_SHADER_TYPE type) const override;
private:
	Shader* m_pVS = nullptr;
	Shader* m_pPS = nullptr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC* m_pGPSD = nullptr;
};

#endif 