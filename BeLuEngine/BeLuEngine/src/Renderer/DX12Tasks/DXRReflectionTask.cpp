#include "stdafx.h"
#include "DXRReflectionTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../SwapChain.h"
#include "../Shader.h"

#include "../Misc/AssetLoader.h"

// Model info
#include "../Geometry/Mesh.h"
#include "../Geometry/Transform.h"
#include "../Geometry/Material.h"


DXRReflectionTask::DXRReflectionTask(
	ID3D12Device5* device,
	ID3D12RootSignature* globalRootSignature,
	const std::wstring& rayGenShaderName, const std::wstring& ClosestHitShaderName, const std::wstring& missShaderName,
	ID3D12StateObject* rayTracingStateObject,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:DXRTask(device, FLAG_THREAD, E_COMMAND_INTERFACE_TYPE::COMPUTE_TYPE, L"ReflectionCommandList")
{
	// Gets deleted in Assetloader
	m_pRayGenShader = AssetLoader::Get()->loadShader(rayGenShaderName	 , E_SHADER_TYPE::DXR);
	m_pHitShader	= AssetLoader::Get()->loadShader(ClosestHitShaderName, E_SHADER_TYPE::DXR);
	m_pMissShader	= AssetLoader::Get()->loadShader(missShaderName		 , E_SHADER_TYPE::DXR);
}

DXRReflectionTask::~DXRReflectionTask()
{
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
