#include "stdafx.h"
#include "DXRReflectionTask.h"

// DX12 Specifics
#include "../Renderer/Shader.h"
#include "../Misc/AssetLoader.h"

// Model info
#include "../../Geometry/Transform.h"

// DXR stuff
#include "../../DXR/ShaderBindingTableGenerator.h"
#include "../../DXR/RaytracingPipelineGenerator.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

TODO("Abstract this class")
#include "../../API/D3D12/D3D12GraphicsManager.h"
#include "../../API/D3D12/D3D12GraphicsBuffer.h"
#include "../../API/IGraphicsTexture.h"
#include "../../API/IGraphicsContext.h"

DXRReflectionTask::DXRReflectionTask(unsigned int dispatchWidth, unsigned int dispatchHeight)
	:GraphicsPass(L"DXR_ReflectionPass")
{
#pragma region PipelineState Object
	// Gets deleted in Assetloader
	m_pRayGenShader = AssetLoader::Get()->loadShader(L"DXR/RayGen.hlsl", E_SHADER_TYPE::DXR);
	m_pHitShader = AssetLoader::Get()->loadShader(L"DXR/Hit.hlsl", E_SHADER_TYPE::DXR);
	m_pMissShader = AssetLoader::Get()->loadShader(L"DXR/Miss.hlsl", E_SHADER_TYPE::DXR);

	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = manager->GetDevice();

	RayTracingPipelineGenerator rtPipelineGenerator(device5);


	// In a way similar to DLLs, each library is associated with a number of
	// exported symbols. This
	// has to be done explicitly in the lines below. Note that a single library
	// can contain an arbitrary number of symbols, whose semantic is given in HLSL
	// using the [shader("xxx")] syntax
	rtPipelineGenerator.AddLibrary(m_pRayGenShader->GetBlob(), { L"RayGen" });
	rtPipelineGenerator.AddLibrary(m_pHitShader->GetBlob(), { L"ClosestHit" });
	rtPipelineGenerator.AddLibrary(m_pMissShader->GetBlob(), { L"Miss" });

	// To be used, each DX12 shader needs a root signature defining which
	// parameters and buffers will be accessed.
#pragma region CreateLocalRootSignatures

	auto createLocalRootSig = [device5](const D3D12_ROOT_SIGNATURE_DESC& rootDesc) -> ID3D12RootSignature*
	{
		BL_ASSERT(rootDesc.Flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

		ID3DBlob* errorMessages = nullptr;
		ID3DBlob* m_pBlob = nullptr;

		HRESULT hr = D3D12SerializeRootSignature(
			&rootDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&m_pBlob,
			&errorMessages);

		if (FAILED(hr) && errorMessages)
		{
			BL_LOG_CRITICAL("Failed to Serialize local RootSignature\n");

			const char* errorMsg = static_cast<const char*>(errorMessages->GetBufferPointer());
			BL_LOG_CRITICAL("%s\n", errorMsg);
		}

		ID3D12RootSignature* pRootSig;
		hr = device5->CreateRootSignature(
			0,
			m_pBlob->GetBufferPointer(),
			m_pBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSig));

		if (FAILED(hr))
		{
			BL_ASSERT_MESSAGE(pRootSig != nullptr, "Failed to create local RootSignature\n");
		}

		BL_SAFE_RELEASE(&m_pBlob);
		return pRootSig;
	};

	// Specify the root signature with its set of parameters
	const unsigned int numStaticSamplers = 0;
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = 0;
	rootDesc.pParameters = nullptr;
	rootDesc.NumStaticSamplers = numStaticSamplers;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	rootDesc.pStaticSamplers = nullptr;

	m_pRayGenSignature = createLocalRootSig(rootDesc);
	m_pRayGenSignature->SetName(L"RayGenRootSig");
	m_pMissSignature = createLocalRootSig(rootDesc);
	m_pMissSignature->SetName(L"MissRootSig");

	// RegisterSpace 0 is dedicated to descriptors, and globalRootsig uses 0-5 on all SRV and UAV and 0-7 on CBV
	D3D12_ROOT_PARAMETER rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::NUM_LOCAL_PARAMS]{};

	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV_T6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV_T6].Descriptor.ShaderRegister = 6;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV_T6].Descriptor.RegisterSpace = 0;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV_T6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_T7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_T7].Descriptor.ShaderRegister = 7;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_T7].Descriptor.RegisterSpace = 0;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_T7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_B8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_B8].Descriptor.ShaderRegister = 8;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_B8].Descriptor.RegisterSpace = 0;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV_B8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	

	rootDesc.NumParameters = E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::NUM_LOCAL_PARAMS;
	rootDesc.pParameters = rootParam;
	m_pHitSignature = createLocalRootSig(rootDesc);
	m_pHitSignature->SetName(L"HitRootSig");

#pragma endregion

	// 3 different shaders can be invoked to obtain an intersection: an
	// intersection shader is called
	// when hitting the bounding box of non-triangular geometry. This is beyond
	// the scope of this tutorial. An any-hit shader is called on potential
	// intersections. This shader can, for example, perform alpha-testing and
	// discard some intersections. Finally, the closest-hit program is invoked on
	// the intersection point closest to the ray origin. Those 3 shaders are bound
	// together into a hit group.

	// Note that for triangular geometry the intersection shader is built-in. An
	// empty any-hit shader is also defined by default, so in our simple case each
	// hit group contains only the closest hit shader. Note that since the
	// exported symbols are defined above the shaders can be simply referred to by
	// name.

	// Hit group for the triangles, with a shader simply interpolating vertex
	// colors
	rtPipelineGenerator.AddHitGroup(L"HitGroup", L"ClosestHit");

	// The following section associates the root signature to each shader.Note
	// that we can explicitly show that some shaders share the same root signature
	// (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
	// to as hit groups, meaning that the underlying intersection, any-hit and
	// closest-hit shaders share the same root signature.
	rtPipelineGenerator.AddRootSignatureAssociation(m_pRayGenSignature, { L"RayGen" });
	rtPipelineGenerator.AddRootSignatureAssociation(m_pHitSignature, { L"HitGroup" });
	rtPipelineGenerator.AddRootSignatureAssociation(m_pMissSignature, { L"Miss" });
	// The payload size defines the maximum size of the data carried by the rays,
	// ie. the the data
	// exchanged between shaders, such as the HitInfo structure in the HLSL code.
	// It is important to keep this value as low as possible as a too high value
	// would result in unnecessary memory consumption and cache trashing.
	rtPipelineGenerator.SetMaxPayloadSize(3 * sizeof(float) + sizeof(unsigned int)); // RGB + recursionDepth

	// Upon hitting a surface, DXR can provide several attributes to the hit. In
	// our sample we just use the barycentric coordinates defined by the weights
	// u,v of the last two vertices of the triangle. The actual barycentrics can
	// be obtained using float3 barycentrics = float3(1.f-u-v, u, v);
	rtPipelineGenerator.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates

	// The raytracing process can shoot rays from existing hit points, resulting
	// in nested TraceRay calls. Our sample code traces only primary rays, which
	// then requires a trace depth of 1. Note that this recursion depth should be
	// kept to a minimum for best performance. Path tracing algorithms can be
	// easily flattened into a simple loop in the ray generation.
	rtPipelineGenerator.SetMaxRecursionDepth(15);

	// Compile the pipeline for execution on the GPU
	m_pStateObject = rtPipelineGenerator.Generate();

	// Cast the state object into a properties object, allowing to later access
	// the shader pointers by name
	m_pStateObject->QueryInterface(IID_PPV_ARGS(&m_pRTStateObjectProps));

	m_pStateObject->SetName(L"DXR_Reflection_StateObject");
#pragma endregion

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
	// ShaderBindingTable
	BL_SAFE_DELETE(m_pSbtGenerator);
	BL_SAFE_DELETE(m_pShaderTableBuffer);

	// Local rootSignatures
	BL_SAFE_RELEASE(&m_pRayGenSignature);
	BL_SAFE_RELEASE(&m_pHitSignature);
	BL_SAFE_RELEASE(&m_pMissSignature);

	TODO("Deferred Deletion");
	Sleep(50);	// Hack
	// StateObject
	BL_SAFE_RELEASE(&m_pStateObject);
	BL_SAFE_RELEASE(&m_pRTStateObjectProps);
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
	m_pSbtGenerator->Generate(static_cast<D3D12GraphicsBuffer*>(m_pShaderTableBuffer)->m_pResource, m_pRTStateObjectProps);
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
		m_pGraphicsContext->SetPipelineState(m_pStateObject);

		RootConstantUints rootConstantUints = {};
		rootConstantUints.index0 = finalColorBuffer->GetUnorderedAccessIndex();	// Write to this texture with this index
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
