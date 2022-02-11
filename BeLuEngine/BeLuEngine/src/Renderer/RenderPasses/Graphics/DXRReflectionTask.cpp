#include "stdafx.h"
#include "DXRReflectionTask.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"
#include "../../Geometry/Transform.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsManager.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"
#include "../Renderer/API/Interface/IGraphicsTexture.h"

// Generic API (RayTracing)
#include "../Renderer/API/Interface/RayTracing/IRayTracingPipelineState.h"
#include "../Renderer/API/Interface/RayTracing/IShaderBindingTable.h"


DXRReflectionTask::DXRReflectionTask(unsigned int dispatchWidth, unsigned int dispatchHeight)
	:GraphicsPass(L"DXR_ReflectionPass")
{
	// PipelineState
	{
		RayTracingPipelineStateDesc rtDesc = {};
		rtDesc.AddShader(L"DXR/RayGen.hlsl", L"RayGen");
		rtDesc.AddShader(L"DXR/Hit.hlsl", L"ClosestHit");
		rtDesc.AddShader(L"DXR/Miss.hlsl", L"Miss" );
		rtDesc.AddShader(L"DXR/ShadowMiss.hlsl", L"ShadowMiss" );

		// No anyhit or intersection
		rtDesc.AddHitgroup(L"HitGroup", L"ClosestHit");

		// Associate rootsignatures
		std::vector<IRayTracingRootSignatureParams> hitGroupRootSigParams(NUM_LOCAL_PARAMS);
		std::vector<IRayTracingRootSignatureParams> emptyRootSigParams;

		hitGroupRootSigParams[RootParamLocal_SRV_T6].rootParamType = BL_ROOT_PARAMETER_SRV;
		hitGroupRootSigParams[RootParamLocal_SRV_T6].shaderRegister = 6;
		hitGroupRootSigParams[RootParamLocal_SRV_T7].rootParamType = BL_ROOT_PARAMETER_SRV;
		hitGroupRootSigParams[RootParamLocal_SRV_T7].shaderRegister = 7;
		hitGroupRootSigParams[RootParamLocal_CBV_B8].rootParamType = BL_ROOT_PARAMETER_CBV;
		hitGroupRootSigParams[RootParamLocal_CBV_B8].shaderRegister = 8;

		rtDesc.AddRootSignatureAssociation(hitGroupRootSigParams, L"HitGroup", L"HitGroup_LocalRootSignature");
		rtDesc.AddRootSignatureAssociation(emptyRootSigParams, L"RayGen", L"RayGenEmpty_LocalRootSignature");
		rtDesc.AddRootSignatureAssociation(emptyRootSigParams, L"Miss", L"MissEmpty_LocalRootSignature");
		rtDesc.AddRootSignatureAssociation(emptyRootSigParams, L"ShadowMiss", L"ShadowMissEmpty_LocalRootSignature");

		// Misc
		rtDesc.SetPayloadSize(3 * sizeof(float) + sizeof(unsigned int)); // RGB + recursionDepth
		rtDesc.SetMaxAttributesSize(2 * sizeof(float));
		rtDesc.SetMaxRecursionDepth(4);

		m_pRayTracingState = IRayTracingPipelineState::Create(rtDesc, L"DXR_Reflection_PipelineState");
	}
	
	// ShaderBindingTable
	m_pShaderBindingTable = IShaderBindingTable::Create(L"DXR_Reflection_ShaderBindingTable");

	// Dispatch Info
	m_DispatchWidth = dispatchWidth;
	m_DispatchHeight = dispatchHeight;
}

DXRReflectionTask::~DXRReflectionTask()
{
	// RayTracing State
	BL_SAFE_DELETE(m_pRayTracingState);

	BL_SAFE_DELETE(m_pShaderBindingTable);
}

void DXRReflectionTask::Execute()
{
	// Updating the sbt every frame
	BL_ASSERT(m_RenderComponents.size());
	createShaderBindingTable();

	IGraphicsTexture* finalColorBuffer = m_GraphicTextures["finalColorBuffer"];
	IGraphicsTexture* depthTexture = m_GraphicTextures["mainDSV"];
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(RaytracedReflections, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(true);

		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B3, m_GraphicBuffers["cbPerFrame"], true);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_GraphicBuffers["cbPerScene"], true);
		m_pGraphicsContext->SetShaderResourceView(RootParam_SRV_T0, m_GraphicBuffers["rawBufferLights"], true);

		// Transitions
		m_pGraphicsContext->ResourceBarrier(depthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(finalColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Bind the raytracing pipeline
		m_pGraphicsContext->SetRayTracingPipelineState(m_pRayTracingState);

		RootConstantUints rootConstantUints = {};
		rootConstantUints.index0 = finalColorBuffer->GetShaderResourceHeapIndex();	// Read from this texture with this index
		rootConstantUints.index1 = finalColorBuffer->GetUnorderedAccessIndex();		// Write to this texture with this index
		m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(RootConstantUints) / 4, &rootConstantUints, 0, true);

		// Dispatch the rays and write to the raytracing output
		m_pGraphicsContext->DispatchRays(m_pShaderBindingTable, m_DispatchWidth, m_DispatchHeight);

		m_pGraphicsContext->UAVBarrier(finalColorBuffer);

		// Transitions
		m_pGraphicsContext->ResourceBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		m_pGraphicsContext->ResourceBarrier(finalColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	m_pGraphicsContext->End();
}

void DXRReflectionTask::SetRenderComponents(const std::vector<RenderComponent>& renderComponents)
{
	m_RenderComponents = renderComponents;
}

void DXRReflectionTask::createShaderBindingTable()
{
	m_pShaderBindingTable->Reset();

	// RayGen, Miss and ShadowMiss are empty, needing no descriptors
	m_pShaderBindingTable->AddShaderRecord(E_SHADER_RECORD_TYPE::RAY_GENERATION_SHADER_RECORD, L"RayGen", {});

	// Note: Make sure they are added in this order, or change the enum E_RT_SECTION_INDICES in GPU_Structs.h
	m_pShaderBindingTable->AddShaderRecord(E_SHADER_RECORD_TYPE::MISS_SHADER_RECORD, L"Miss", {});
	m_pShaderBindingTable->AddShaderRecord(E_SHADER_RECORD_TYPE::MISS_SHADER_RECORD, L"ShadowMiss", {});

	// Add Parameters to hitgroup so that the triangleData can be accessed within them
	for (const RenderComponent& rc : m_RenderComponents)
	{
		m_pShaderBindingTable->AddShaderRecord(E_SHADER_RECORD_TYPE::HIT_GROUP_SHADER_RECORD, L"HitGroup",
			{
				rc.mc->GetSlotInfoByteAdressBufferDXR(),	// SlotInfoRawBuffer
				rc.mc->GetMaterialByteAdressBuffer(),		// MaterialData
				rc.tc->GetTransform()->m_pConstantBuffer	// MATRICES_PER_OBJECT_STRUCT
			});
	}

	// Generate the SBT on a CPU buffer
	m_pShaderBindingTable->GenerateShaderBindingTable(m_pRayTracingState);
}
