#include "stdafx.h"
#include "DXRReflectionTask.h"

// Core
#include "../Misc/Log.h"

// DX12 Specifics
//#include "../Camera/BaseCamera.h"
//#include "../CommandInterface.h"
//#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
//#include "../PipelineState/PipelineState.h"
//#include "../RenderView.h"
//#include "../SwapChain.h"
#include "../Shader.h"
#include "../Misc/AssetLoader.h"

// Model info
//#include "../Geometry/Mesh.h"
#include "../Geometry/Transform.h"
//#include "../Geometry/Material.h"

// DXR stuff
#include "../DXR/ShaderBindingTableGenerator.h"
#include "../DXR/RaytracingPipelineGenerator.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

DXRReflectionTask::DXRReflectionTask(
	ID3D12Device5* device,
	ID3D12RootSignature* globalRootSignature,
	unsigned int FLAG_THREAD)
	:DXRTask(device, FLAG_THREAD, E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE, L"ReflectionCommandList")
{
#pragma region PipelineState Object
	// Gets deleted in Assetloader
	m_pRayGenShader = AssetLoader::Get()->loadShader(L"DXR/RayGen.hlsl"	, E_SHADER_TYPE::DXR);
	m_pHitShader	= AssetLoader::Get()->loadShader(L"DXR/Hit.hlsl"	, E_SHADER_TYPE::DXR);
	m_pMissShader	= AssetLoader::Get()->loadShader(L"DXR/Miss.hlsl"	, E_SHADER_TYPE::DXR);

	RayTracingPipelineGenerator rtPipelineGenerator(device);


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

	auto createLocalRootSig = [device](const D3D12_ROOT_SIGNATURE_DESC& rootDesc) -> ID3D12RootSignature*
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
		hr = device->CreateRootSignature(
			0,
			m_pBlob->GetBufferPointer(),
			m_pBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSig));

		if (FAILED(hr))
		{
			BL_ASSERT_MESSAGE(pRootSig != nullptr, "Failed to create local RootSignature\n");
		}

		return pRootSig;
		//BL_SAFE_RELEASE(&m_pBlob);
	};

	// Specify the root signature with its set of parameters
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = 0;
	rootDesc.pParameters = nullptr;
	rootDesc.NumStaticSamplers = 0;
	rootDesc.pStaticSamplers = nullptr;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	m_pRayGenSignature	= createLocalRootSig(rootDesc);
	m_pMissSignature	= createLocalRootSig(rootDesc);

	// RegisterSpace 0 is dedicated to descriptors, and globalRootsig uses 0-5 on all SRV,CBV and UAV
	D3D12_ROOT_PARAMETER rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::NUM_LOCAL_PARAMS]{};
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV0].Descriptor.ShaderRegister = 6;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV0].Descriptor.RegisterSpace = 0;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_SRV0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV0].Descriptor.ShaderRegister = 9;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV0].Descriptor.RegisterSpace = 0;
	rootParam[E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::RootParamLocal_CBV0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootDesc.NumParameters = E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION::NUM_LOCAL_PARAMS;
	rootDesc.pParameters = rootParam;
	m_pHitSignature = createLocalRootSig(rootDesc);

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
	rtPipelineGenerator.SetMaxPayloadSize(4 * sizeof(float)); // RGB + distance

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
	rtPipelineGenerator.SetMaxRecursionDepth(2);

	// Compile the pipeline for execution on the GPU
	m_pStateObject = rtPipelineGenerator.Generate(globalRootSignature);

	// Cast the state object into a properties object, allowing to later access
	// the shader pointers by name
	m_pStateObject->QueryInterface(IID_PPV_ARGS(&m_pRTStateObjectProps));
#pragma endregion

#pragma region ShaderBindingTable
	// This will get generated from outside of this class (by calling CreateShaderBindingTable), whenever sbt needs updating..
	// It needs updating whenever instances has been added/removed from the rtScene.
	// However the instance to the class will stay the same throughtout application lifetime.
	m_pSbtGenerator = new ShaderBindingTableGenerator();
#pragma endregion
}

DXRReflectionTask::~DXRReflectionTask()
{
	// ShaderBindingTable
	BL_SAFE_DELETE(m_pSbtGenerator);
	BL_SAFE_DELETE(m_pSbtStorage);

	// Local rootSignatures
	BL_SAFE_RELEASE(&m_pRayGenSignature);
	BL_SAFE_RELEASE(&m_pHitSignature);
	BL_SAFE_RELEASE(&m_pMissSignature);

	// StateObject
	BL_SAFE_RELEASE(&m_pStateObject);
	BL_SAFE_RELEASE(&m_pRTStateObjectProps);
}

void DXRReflectionTask::CreateShaderBindingTable(ID3D12Device5* device, const std::vector<RenderComponent>& rayTracedRenderComponents)
{
	BL_SAFE_DELETE(m_pSbtStorage);

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
			(void*)rc.tc->GetTransform()->m_pCB->GetDefaultResource()->GetGPUVirtualAdress(),	// Unique per instance
			(void*)rc.mc->GetByteAdressInfoDXR()->GetDefaultResource()->GetGPUVirtualAdress()	// Unique per modelComponent
		});
	}

	// Compute the size of the SBT given the number of shaders and their
	// parameters
	uint32_t sbtSize = m_pSbtGenerator->ComputeSBTSize();

	// Create the SBT on the upload heap. This is required as the helper will use
	// mapping to write the SBT contents. After the SBT compilation it could be
	// copied to the default heap for performance.

	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Alignment = 0;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.Height = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Width = sbtSize;

	D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATE_GENERIC_READ;
	m_pSbtStorage = new Resource(device, sbtSize, RESOURCE_TYPE::UPLOAD, L"ShaderBindingTable_Resource", D3D12_RESOURCE_FLAG_NONE, &startState);

	// TODO: Add after stateObject is completed
	// Compile the SBT from the shader and parameters info
	m_pSbtGenerator->Generate(m_pSbtStorage->GetID3D12Resource1(), m_pRTStateObjectProps);
}

void DXRReflectionTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(RaytracedReflections, commandList);
	}
	commandList->Close();
}
