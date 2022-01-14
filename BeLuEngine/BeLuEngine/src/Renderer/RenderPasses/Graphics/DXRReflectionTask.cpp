#include "stdafx.h"
#include "DXRReflectionTask.h"

#include "../Renderer/Shaders/Shader.h"
#include "../Misc/AssetLoader.h"

// Model info
#include "../../Geometry/Transform.h"

#include "../Renderer/API/D3D12/DXR/D3D12RayTracingPipelineState.h"

TODO("Include Generic API");
// DXR stuff
#include "../Renderer/API/D3D12/DXR/RaytracingPipelineGenerator.h"
#include "../Renderer/API/D3D12/DXR/ShaderBindingTableGenerator.h"
// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

TODO("Abstract this class")
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12GraphicsBuffer.h"
#include "../Renderer/API/Interface/IGraphicsTexture.h"
#include "../Renderer/API/D3D12/D3D12GraphicsContext.h"
#include "../Renderer/API/D3D12/D3D12DescriptorHeap.h"

DXRReflectionTask::DXRReflectionTask(unsigned int dispatchWidth, unsigned int dispatchHeight)
	:GraphicsPass(L"DXR_ReflectionPass")
{
	{
		RayTracingPSDesc rtDesc = {};
		rtDesc.AddShader(L"DXR/RayGen.hlsl", L"RayGen");
		rtDesc.AddShader(L"DXR/Hit.hlsl", L"ClosestHit");
		rtDesc.AddShader(L"DXR/Miss.hlsl", L"Miss" );

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

		rtDesc.AddRootSignatureAssociation(hitGroupRootSigParams, L"HitGroup", L"HitGroupRootSignature");
		rtDesc.AddRootSignatureAssociation(emptyRootSigParams, L"RayGen", L"RayGenEmptyRootSignature");
		rtDesc.AddRootSignatureAssociation(emptyRootSigParams, L"Miss", L"MissEmptyRootSignature");

		// Misc
		rtDesc.SetPayloadSize(3 * sizeof(float) + sizeof(unsigned int)); // RGB + recursionDepth
		rtDesc.SetMaxAttributesSize(2 * sizeof(float));
		rtDesc.SetMaxRecursionDepth(2);

		m_pRayTracingState = IRayTracingPipelineState::Create(rtDesc, L"DXR_ReflectionPipelineState");
	}
	
#pragma region ShaderBindingTable
	// This will get generated from outside of this class (by calling CreateShaderBindingTable), whenever sbt needs updating..
	// It needs updating whenever instances has been added/removed from the rtScene.
	// However the instance to the class will stay the same throughtout application lifetime.
	m_pSbtGenerator = new ShaderBindingTableGenerator();
#pragma endregion

	// Rest
	m_DispatchWidth = dispatchWidth;
	m_DispatchHeight = dispatchHeight;
}

DXRReflectionTask::~DXRReflectionTask()
{
	// RayTracing State
	BL_SAFE_DELETE(m_pRayTracingState);

	// ShaderBindingTable
	BL_SAFE_DELETE(m_pSbtGenerator);
	BL_SAFE_DELETE(m_pShaderTableBuffer);
}

void DXRReflectionTask::CreateShaderBindingTable(const std::vector<RenderComponent>& rayTracedRenderComponents)
{
	BL_SAFE_DELETE(m_pShaderTableBuffer);

	// The SBT helper class collects calls to Add*Program.  If called several
	// times, the helper must be emptied before re-adding shaders.
	m_pSbtGenerator->Reset();

	// The ray generation only uses heap data
	m_pSbtGenerator->AddRayGenerationProgram(L"RayGen", {});

	// The miss and hit shaders do not access any external resources: instead they
	// communicate their results through the ray payload
	m_pSbtGenerator->AddMissProgram(L"Miss", {});

	// Adding the triangle hit shader
	for (const RenderComponent& rc : rayTracedRenderComponents)
	{
		m_pSbtGenerator->AddHitGroup(L"HitGroup",
		{
			(void*)static_cast<D3D12GraphicsBuffer*>(rc.mc->GetSlotInfoByteAdressBufferDXR())->m_pResource->GetGPUVirtualAddress(),	// SlotInfoRawBuffer
			(void*)static_cast<D3D12GraphicsBuffer*>(rc.mc->GetMaterialByteAdressBuffer())->m_pResource->GetGPUVirtualAddress(),	// MaterialData
			(void*)static_cast<D3D12GraphicsBuffer*>(rc.tc->GetTransform()->m_pConstantBuffer)->m_pResource->GetGPUVirtualAddress()	// MATRICES_PER_OBJECT_STRUCT
		});
	}

	// Compute the size of the SBT given the number of shaders and their
	// parameters
	uint32_t sbtSize = m_pSbtGenerator->ComputeSBTSize();

	// Create the SBT on the upload heap. This is required as the helper will use
	// mapping to write the SBT contents. After the SBT compilation it could be
	// copied to the default heap for performance.

	BL_SAFE_DELETE(m_pShaderTableBuffer);
	m_pShaderTableBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::CPUBuffer, sbtSize, 1, DXGI_FORMAT_UNKNOWN, L"ShaderBindingTable_CPUBUFFER");

	// Compile the SBT from the shader and parameters info
	m_pSbtGenerator->Generate(static_cast<D3D12GraphicsBuffer*>(m_pShaderTableBuffer)->m_pResource, static_cast<D3D12RayTracingPipelineState*>(m_pRayTracingState)->GetTempStatePropsObject());
}

void DXRReflectionTask::Execute()
{
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
		m_pGraphicsContext->ResourceBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(finalColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Setup the raytracing task
		D3D12_DISPATCH_RAYS_DESC desc = {};

		// The layout of the SBT is as follows: ray generation shader, miss
		// shader, hit groups. As seen in the CreateShaderBindingTable method,
		// all SBT entries of a given type have the same size to allow a fixed stride.

		// Ray generation section
		uint32_t rayGenerationSectionSizeInBytes = m_pSbtGenerator->GetRayGenSectionSize();
		desc.RayGenerationShaderRecord.StartAddress = static_cast<D3D12GraphicsBuffer*>(m_pShaderTableBuffer)->m_pResource->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

		// Miss section
		uint32_t missSectionSizeInBytes = m_pSbtGenerator->GetMissSectionSize();
		desc.MissShaderTable.StartAddress = static_cast<D3D12GraphicsBuffer*>(m_pShaderTableBuffer)->m_pResource->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
		desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
		desc.MissShaderTable.StrideInBytes = m_pSbtGenerator->GetMissEntrySize();

		// Hit group section
		uint32_t hitGroupsSectionSize = m_pSbtGenerator->GetHitGroupSectionSize();
		desc.HitGroupTable.StartAddress = static_cast<D3D12GraphicsBuffer*>(m_pShaderTableBuffer)->m_pResource->GetGPUVirtualAddress() +
			rayGenerationSectionSizeInBytes +
			missSectionSizeInBytes;
		desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
		desc.HitGroupTable.StrideInBytes = m_pSbtGenerator->GetHitGroupEntrySize();

		// Dimensions of the image to render
		desc.Width = m_DispatchWidth;
		desc.Height = m_DispatchHeight;
		desc.Depth = 1;

		// Bind the raytracing pipeline
		m_pGraphicsContext->SetRayTracingPipelineState(m_pRayTracingState);

		RootConstantUints rootConstantUints = {};
		rootConstantUints.index0 = finalColorBuffer->GetShaderResourceHeapIndex();	// Read from this texture with this index
		rootConstantUints.index1 = finalColorBuffer->GetUnorderedAccessIndex();		// Write to this texture with this index
		m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(RootConstantUints) / 4, &rootConstantUints, 0, true);

		// Dispatch the rays and write to the raytracing output
		m_pGraphicsContext->DispatchRays(desc);

		m_pGraphicsContext->UAVBarrier(finalColorBuffer);

		// Transitions
		m_pGraphicsContext->ResourceBarrier(depthTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		m_pGraphicsContext->ResourceBarrier(finalColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	m_pGraphicsContext->End();
}
