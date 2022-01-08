#include "stdafx.h"
#include "D3D12GraphicsPipelineState.h"

#include "../Renderer/Shaders/Shader.h"

#include "D3D12GraphicsManager.h"
#include "../Misc/AssetLoader.h"

D3D12GraphicsPipelineState::D3D12GraphicsPipelineState(const PSODesc& desc, const std::wstring& name)
{
	std::wstring psoName = name.c_str();
	psoName.append(L"_PipelineState");

#ifdef DEBUG
	m_DebugName = psoName;
#endif

	D3D12GraphicsManager* d3d12Manager = D3D12GraphicsManager::GetInstance();
	D3D12_PSO_STREAM psoStream = {};

	psoStream.rootSignature = d3d12Manager->GetGlobalRootSignature();

	bool isComputeState = false;
	// Shaders
	auto setShaderByteCode = [&psoStream](Shader* shader, E_SHADER_TYPE shaderType)
	{
		IDxcBlob* shaderBlob = shader->GetBlob();
		switch (shaderType)
		{
		case E_SHADER_TYPE::VS:
			psoStream.VS.pShaderBytecode = shaderBlob->GetBufferPointer();
			psoStream.VS.BytecodeLength  = shaderBlob->GetBufferSize();
			break;
		case E_SHADER_TYPE::PS:
			psoStream.PS.pShaderBytecode = shaderBlob->GetBufferPointer();
			psoStream.PS.BytecodeLength = shaderBlob->GetBufferSize();
			break;
		case E_SHADER_TYPE::DS:
			psoStream.DS.pShaderBytecode = shaderBlob->GetBufferPointer();
			psoStream.DS.BytecodeLength = shaderBlob->GetBufferSize();
			break;
		case E_SHADER_TYPE::HS:
			psoStream.HS.pShaderBytecode = shaderBlob->GetBufferPointer();
			psoStream.HS.BytecodeLength = shaderBlob->GetBufferSize();
			break;
		case E_SHADER_TYPE::GS:
			psoStream.GS.pShaderBytecode = shaderBlob->GetBufferPointer();
			psoStream.GS.BytecodeLength = shaderBlob->GetBufferSize();
			break;
		case E_SHADER_TYPE::CS:
			psoStream.CS.pShaderBytecode = shaderBlob->GetBufferPointer();
			psoStream.CS.BytecodeLength = shaderBlob->GetBufferSize();
			break;
		//case E_SHADER_TYPE::AS:
		//	psoStream.AS.pShaderBytecode = shaderBlob->GetBufferPointer();
		//	psoStream.AS.BytecodeLength = shaderBlob->GetBufferSize();
		//	break;
		//case E_SHADER_TYPE::MS:
		//	psoStream.MS.pShaderBytecode = shaderBlob->GetBufferPointer();
		//	psoStream.MS.BytecodeLength = shaderBlob->GetBufferSize();
		//	break;
		default:
			return false;
		}
	};

	for (unsigned int i = 0; i < E_SHADER_TYPE::NUM_SHADER_TYPES; i++)
	{
		if (desc.m_Shaders[i] != L"")
		{
			Shader* shader = AssetLoader::Get()->loadShader(desc.m_Shaders[i], static_cast<E_SHADER_TYPE>(i));

			setShaderByteCode(shader, static_cast<E_SHADER_TYPE>(i));

			if (static_cast<E_SHADER_TYPE>(i) == E_SHADER_TYPE::CS)
				isComputeState = true;
		}
	}
	
	// Compute states doesn't have any of this
	if (isComputeState == false)
	{
		// RenderTargets
		psoStream.renderTargetFormats.NumRenderTargets = desc.m_NumRenderTargets;
		for (unsigned int i = 0; i < desc.m_NumRenderTargets; i++)
		{
			psoStream.renderTargetFormats.RTFormats[i] = ConvertBLFormatToD3D12Format(desc.m_RenderTargetFormats[i]);
		}

		// Depth
		psoStream.depthStencilFormat = ConvertBLFormatToD3D12Format(desc.m_DepthStencilFormat);

		// Blend
		// These are not supported yet
		psoStream.blendDesc.desc.AlphaToCoverageEnable = false;
		psoStream.blendDesc.desc.IndependentBlendEnable = false;
		
		for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		{
			BL_RENDER_TARGET_BLEND_DESC blendDesc = desc.m_RenderTargetBlendDescs[i];
			BL_ASSERT(!(blendDesc.enableBlend && blendDesc.enableLogicOP));

			psoStream.blendDesc.desc.RenderTarget[i].BlendEnable = blendDesc.enableBlend;
			psoStream.blendDesc.desc.RenderTarget[i].SrcBlend = ConvertBLBlendToD3D12Blend(blendDesc.srcBlend);
			psoStream.blendDesc.desc.RenderTarget[i].DestBlend = ConvertBLBlendToD3D12Blend(blendDesc.destBlend);
			psoStream.blendDesc.desc.RenderTarget[i].BlendOp = ConvertBLBlendOPToD3D12BlendOP(blendDesc.blendOp);
			psoStream.blendDesc.desc.RenderTarget[i].SrcBlendAlpha = ConvertBLBlendToD3D12Blend(blendDesc.srcBlendAlpha);
			psoStream.blendDesc.desc.RenderTarget[i].DestBlendAlpha = ConvertBLBlendToD3D12Blend(blendDesc.destBlendAlpha);
			psoStream.blendDesc.desc.RenderTarget[i].BlendOpAlpha = ConvertBLBlendOPToD3D12BlendOP(blendDesc.blendOpAlpha);
			psoStream.blendDesc.desc.RenderTarget[i].RenderTargetWriteMask = blendDesc.renderTargetWriteMask;

			psoStream.blendDesc.desc.RenderTarget[i].LogicOpEnable = blendDesc.enableLogicOP;
			psoStream.blendDesc.desc.RenderTarget[i].LogicOp = ConvertBLLogicOPToD3D12LogicOP(blendDesc.logicOp);
		}
		
		psoStream.sampleMask = UINT_MAX;
		psoStream.primTop = ConvertBLPrimTopPipelineStateToD3D12PrimTopPipelineState(desc.m_PrimTop);

		// Rasterizer
		psoStream.rasterizerDesc.desc = {};
		psoStream.rasterizerDesc.desc.CullMode = ConvertBLCullModeToD3D12CullMode(desc.m_CullMode);
		psoStream.rasterizerDesc.desc.FillMode = ConvertBLFillModeToD3D12FillMode(desc.m_FillMode);
		psoStream.rasterizerDesc.desc.DepthClipEnable = false;	TODO("Set this to true?");

		// Depth desc
		psoStream.depthStencilDesc1.DepthEnable = desc.m_DepthStencilDesc.enableDepth;
		psoStream.depthStencilDesc1.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(desc.m_DepthStencilDesc.depthWriteMask);
		psoStream.depthStencilDesc1.DepthFunc = ConvertBLComparisonFuncToD3D12ComparisonFunc(desc.m_DepthStencilDesc.depthComparisonFunc);
		psoStream.depthStencilDesc1.DepthBoundsTestEnable = false;

		// Stencil desc
		psoStream.depthStencilDesc1.StencilEnable = desc.m_DepthStencilDesc.enableStencil;
		psoStream.depthStencilDesc1.StencilReadMask = desc.m_DepthStencilDesc.stencilReadMask;
		psoStream.depthStencilDesc1.StencilWriteMask = desc.m_DepthStencilDesc.stencilWriteMask;
		psoStream.depthStencilDesc1.FrontFace.StencilFailOp = ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.frontFace.stencilFailOp);
		psoStream.depthStencilDesc1.FrontFace.StencilDepthFailOp = ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.frontFace.stencilDepthFailOp);
		psoStream.depthStencilDesc1.FrontFace.StencilPassOp = ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.frontFace.stencilPassOp);
		psoStream.depthStencilDesc1.FrontFace.StencilFunc = ConvertBLComparisonFuncToD3D12ComparisonFunc(desc.m_DepthStencilDesc.frontFace.stencilComparisonFunc);
		psoStream.depthStencilDesc1.BackFace.StencilFailOp = ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.backFace.stencilFailOp);
		psoStream.depthStencilDesc1.BackFace.StencilDepthFailOp = ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.backFace.stencilDepthFailOp);
		psoStream.depthStencilDesc1.BackFace.StencilPassOp = ConvertBLStencilOPToD3D12StencilOP(desc.m_DepthStencilDesc.backFace.stencilPassOp);
		psoStream.depthStencilDesc1.BackFace.StencilFunc = ConvertBLComparisonFuncToD3D12ComparisonFunc(desc.m_DepthStencilDesc.backFace.stencilComparisonFunc);
	}

	// GPU
	psoStream.nodeMask.NodeMask = 0;

	const D3D12_PIPELINE_STATE_STREAM_DESC psoStreamDesc{ sizeof(psoStream), &psoStream };
	HRESULT hr = d3d12Manager->GetDevice()->CreatePipelineState(&psoStreamDesc, IID_PPV_ARGS(&m_pPSO));
	d3d12Manager->CHECK_HRESULT(hr);

	m_pPSO->SetName(psoName.c_str());
}

D3D12GraphicsPipelineState::~D3D12GraphicsPipelineState()
{
	D3D12GraphicsManager::GetInstance()->AddD3D12ObjectToDefferedDeletion(m_pPSO);
}

StreamingRasterizerDesc::StreamingRasterizerDesc()
{
	desc.CullMode = D3D12_CULL_MODE_BACK;
	desc.FillMode = D3D12_FILL_MODE_SOLID;
}
