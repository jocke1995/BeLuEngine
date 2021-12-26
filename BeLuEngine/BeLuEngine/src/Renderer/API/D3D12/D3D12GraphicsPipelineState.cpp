#include "stdafx.h"
#include "D3D12GraphicsPipelineState.h"

#include "D3D12GraphicsManager.h"
#include "../Misc/AssetLoader.h"

D3D12GraphicsPipelineState::D3D12GraphicsPipelineState(const PSODesc& desc, const std::wstring& name)
{
#ifdef DEBUG
	m_DebugName = name;
#endif

	D3D12GraphicsManager* d3d12Manager = D3D12GraphicsManager::GetInstance();
	D3D12_PSO_STREAM psoStream = {};

	psoStream.rootSignature = d3d12Manager->GetGlobalRootSignature();

	// Shaders
	for (unsigned int i = 0; i < E_SHADER_TYPE::NUM_SHADER_TYPES; i++)
	{
		if (desc.m_Shaders[i] != L"")
			AssetLoader::Get()->loadShader(desc.m_Shaders[i], static_cast<E_SHADER_TYPE>(i));
	}

	// RenderTargets
	psoStream.renderTargetFormats.NumRenderTargets = desc.m_NumRenderTargets;
	for (unsigned int i = 0; i < desc.m_NumRenderTargets; i++)
	{
		psoStream.renderTargetFormats.RTFormats[i] = ConvertBLFormatToD3D12Format(desc.m_RenderTargetFormats[i]);
	}

	// Depth
	psoStream.depthStencilFormat = ConvertBLFormatToD3D12Format(desc.m_DepthStencilFormat);

#pragma region Blend
	// These are not supported yet
	psoStream.blendDesc.AlphaToCoverageEnable = false;
	psoStream.blendDesc.IndependentBlendEnable = false;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		BL_RENDER_TARGET_BLEND_DESC blendDesc = desc.m_RenderTargetBlendDescs[i];
		if (blendDesc.enableBlend == true)
		{
			psoStream.blendDesc.RenderTarget[i].SrcBlend		= ConvertBLBlendToD3D12Blend(blendDesc.srcBlend);
			psoStream.blendDesc.RenderTarget[i].DestBlend		= ConvertBLBlendToD3D12Blend(blendDesc.destBlend);
			psoStream.blendDesc.RenderTarget[i].BlendOp			= ConvertBLBlendOPToD3D12BlendOP(blendDesc.blendOp);
			psoStream.blendDesc.RenderTarget[i].SrcBlendAlpha	= ConvertBLBlendToD3D12Blend(blendDesc.srcBlendAlpha);
			psoStream.blendDesc.RenderTarget[i].DestBlendAlpha	= ConvertBLBlendToD3D12Blend(blendDesc.destBlendAlpha);
			psoStream.blendDesc.RenderTarget[i].BlendOpAlpha	= ConvertBLBlendOPToD3D12BlendOP(blendDesc.blendOpAlpha);
			psoStream.blendDesc.RenderTarget[i].RenderTargetWriteMask = blendDesc.renderTargetWriteMask;
		}
		else if (blendDesc.logicOp == true)
		{
			psoStream.blendDesc.RenderTarget[i].LogicOpEnable = true;
			psoStream.blendDesc.RenderTarget[i].LogicOp = ConvertBLLogicOPToD3D12LogicOP(blendDesc.logicOp);
		}
	}
#pragma endregion

	psoStream.sampleMask = UINT_MAX;
	psoStream.primTop = ConvertBLPrimTopToD3D12PrimTop(desc.m_PrimTop);

	// Rasterizer
	psoStream.rasterizerDesc = {};
	psoStream.rasterizerDesc.CullMode = ConvertBLCullModeToD3D12CullMode(desc.m_CullMode);
	psoStream.rasterizerDesc.FillMode = ConvertBLFillModeToD3D12FillMode(desc.m_FillMode);
	psoStream.rasterizerDesc.DepthClipEnable = false;	TODO("Set this to true?");

	// Depth desc
	psoStream.depthStencilDesc.DepthEnable = desc.m_DepthStencilDesc.enableDepth;
	psoStream.depthStencilDesc.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(desc.m_DepthStencilDesc.depthWriteMask);
	psoStream.depthStencilDesc.DepthFunc = ConvertBLComparisonFuncToD3D12ComparisonFunc(desc.m_DepthStencilDesc.depthComparisonFunc);

	// Stencil desc
	psoStream.depthStencilDesc.StencilEnable = desc.m_DepthStencilDesc.enableStencil;
	psoStream.depthStencilDesc.StencilReadMask = desc.m_DepthStencilDesc.stencilReadMask;
	psoStream.depthStencilDesc.StencilWriteMask = desc.m_DepthStencilDesc.stencilWriteMask;
	psoStream.depthStencilDesc.FrontFace.StencilFailOp		= ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.frontFace.stencilFailOp);
	psoStream.depthStencilDesc.FrontFace.StencilDepthFailOp = ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.frontFace.stencilDepthFailOp);
	psoStream.depthStencilDesc.FrontFace.StencilPassOp		= ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.frontFace.stencilPassOp);
	psoStream.depthStencilDesc.FrontFace.StencilFunc		= ConvertBLComparisonFuncToD3D12ComparisonFunc(desc.m_DepthStencilDesc.frontFace.stencilComparisonFunc);
	psoStream.depthStencilDesc.BackFace.StencilFailOp		= ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.backFace.stencilFailOp);
	psoStream.depthStencilDesc.BackFace.StencilDepthFailOp	= ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.backFace.stencilDepthFailOp);
	psoStream.depthStencilDesc.BackFace.StencilPassOp		= ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.backFace.stencilPassOp);
	psoStream.depthStencilDesc.BackFace.StencilFunc			= ConvertBLComparisonFuncToD3D12ComparisonFunc(desc.m_DepthStencilDesc.backFace.stencilComparisonFunc);

	// SampleDesc
	psoStream.sampleDesc.Count = 1;
	psoStream.sampleDesc.Quality = 0;

	// GPU
	psoStream.nodeMask.NodeMask = 0;

	const D3D12_PIPELINE_STATE_STREAM_DESC psoStreamDesc{ sizeof(psoStream), &psoStream };
	HRESULT hr = d3d12Manager->GetDevice()->CreatePipelineState(&psoStreamDesc, IID_PPV_ARGS(&m_pPSO));
	d3d12Manager->SucceededHRESULT(hr);

	std::wstring psoName = name.c_str();
	psoName.append(L"_PipelineState");
	m_pPSO->SetName(psoName.c_str());
}

D3D12GraphicsPipelineState::~D3D12GraphicsPipelineState()
{
	D3D12GraphicsManager::GetInstance()->AddD3D12ObjectToDefferedDeletion(m_pPSO);
}
