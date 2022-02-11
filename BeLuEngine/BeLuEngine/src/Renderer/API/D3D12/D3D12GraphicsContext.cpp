#include "stdafx.h"
#include "D3D12GraphicsContext.h"

// Common DX12
#include "D3D12GraphicsManager.h"
#include "D3D12GraphicsTexture.h"
#include "D3D12GraphicsBuffer.h"
#include "D3D12GraphicsPipelineState.h"
#include "D3D12DescriptorHeap.h"

// DXR
#include "DXR/D3D12TopLevelAS.h"
#include "DXR/D3D12BottomLevelAS.h"
#include "DXR/D3D12RayTracingPipelineState.h"
#include "DXR/D3D12ShaderBindingTable.h"

//ImGui
#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

D3D12GraphicsContext::D3D12GraphicsContext(const std::wstring& name)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device = graphicsManager->GetDevice();

#ifdef DEBUG
	m_Name = name;
#endif
	
	HRESULT hr;
#pragma region CreateMainCommandInterface
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocators[i]));

		if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to Create CommandAllocator: %S\n", name);
		}

		std::wstring counter = std::to_wstring(i);
		hr = m_pCommandAllocators[i]->SetName((name + L"_CmdAlloc_" + counter).c_str());
		
		if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to SetName on commandAllocator: %S\n", name);
		}
	}

	hr = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_pCommandAllocators[0],
		nullptr,
		IID_PPV_ARGS(&m_pCommandList));

	hr = m_pCommandList->SetName((name + L"_CmdList").c_str());
	if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to Create CommandList: %S\n", name);
	}

	hr = m_pCommandList->Close();
	if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to SetName on CommandList: %S\n", name);
	}

#pragma endregion

#pragma region CreateTransitionCommandInterface
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pTransitionCommandAllocators[i]));

		if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to Create TransitionCommandAllocator: %S\n", name);
		}

		std::wstring counter = std::to_wstring(i);
		hr = m_pTransitionCommandAllocators[i]->SetName((name + L"_TransitionCmdAlloc_" + counter).c_str());

		if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to SetName on TransitionCommandAllocator: %S\n", name);
		}
	}

	hr = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_pTransitionCommandAllocators[0],
		nullptr,
		IID_PPV_ARGS(&m_pTransitionCommandList));

	hr = m_pTransitionCommandList->SetName((name + L"_TransitionCmdList").c_str());
	if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to Create TransitionCommandList: %S\n", name);
	}

	hr = m_pTransitionCommandList->Close();
	if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to SetName on TransitionCommandList: %S\n", name);
	}

#pragma endregion
}

D3D12GraphicsContext::~D3D12GraphicsContext()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		graphicsManager->AddIUknownForDefferedDeletion(m_pCommandAllocators[i]);
		graphicsManager->AddIUknownForDefferedDeletion(m_pTransitionCommandAllocators[i]);
	}
	graphicsManager->AddIUknownForDefferedDeletion(m_pCommandList);
	graphicsManager->AddIUknownForDefferedDeletion(m_pTransitionCommandList);
}

void D3D12GraphicsContext::Begin()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	unsigned int index = graphicsManager->GetCommandInterfaceIndex();

	// Reset Commandinterface
	D3D12GraphicsManager::CHECK_HRESULT(m_pCommandAllocators[index]->Reset());
	D3D12GraphicsManager::CHECK_HRESULT(m_pCommandList->Reset(m_pCommandAllocators[index], NULL));
}

void D3D12GraphicsContext::SetupBindings(bool isComputePipeline)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	// DescriptorHeap
	ID3D12DescriptorHeap* d3d12DescriptorHeap = graphicsManager->GetMainDescriptorHeap()->GetID3D12DescriptorHeap();
	m_pCommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	// Rootsignature and DescriptorTables
	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootSignature(graphicsManager->GetGlobalRootSignature());

		m_pCommandList->SetComputeRootDescriptorTable(dtSRV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetComputeRootDescriptorTable(dtCBV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetComputeRootDescriptorTable(dtUAV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
	else
	{
		m_pCommandList->SetGraphicsRootSignature(graphicsManager->GetGlobalRootSignature());

		m_pCommandList->SetGraphicsRootDescriptorTable(dtSRV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetGraphicsRootDescriptorTable(dtCBV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetGraphicsRootDescriptorTable(dtUAV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
}

void D3D12GraphicsContext::End()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12GraphicsManager::CHECK_HRESULT(m_pCommandList->Close());
}

void D3D12GraphicsContext::UploadTexture(IGraphicsTexture* graphicsTexture)
{
	IGraphicsBuffer* tempUploadBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::CPUBuffer, graphicsTexture->GetSize(), 1, DXGI_FORMAT_UNKNOWN, L"TempUploadResource");

	ID3D12Resource* uploadHeap = static_cast<D3D12GraphicsBuffer*>(tempUploadBuffer)->m_pResource;
	ID3D12Resource* defaultHeap = static_cast<D3D12GraphicsTexture*>(graphicsTexture)->m_pResource;

	// Transfer the data
	UpdateSubresources(m_pCommandList,
		defaultHeap, uploadHeap,
		0, 0, static_cast<D3D12GraphicsTexture*>(graphicsTexture)->GetTempSubresources()->size(),
		static_cast<D3D12GraphicsTexture*>(graphicsTexture)->GetTempSubresources()->data());

	// Deferred deletion of the buffer, it stays alive up until NUM_SWAP_BUFFERS-frames ahead so the copy will be guaranteed to be completed by then.
	BL_SAFE_DELETE(tempUploadBuffer);
}

void D3D12GraphicsContext::UploadBuffer(IGraphicsBuffer* graphicsBuffer, const void* data)
{
	BL_ASSERT(data);

	unsigned __int64 sizeInBytes = graphicsBuffer->GetSize();
	D3D12GraphicsBuffer* d3d12Buffer = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer);
	ID3D12Resource* defaultHeap = d3d12Buffer->m_pResource;

	// Set the data in the perFrameImmediate buffer
	DynamicDataParams dynamicDataParams = D3D12GraphicsManager::GetInstance()->SetDynamicData(sizeInBytes, data);

	//BL_LOG_INFO("%d\n", dynamicDataParams.offsetFromStart);
	m_pCommandList->CopyBufferRegion(defaultHeap, 0, dynamicDataParams.uploadResource, dynamicDataParams.offsetFromStart, sizeInBytes);
}

void D3D12GraphicsContext::SetPipelineState(IGraphicsPipelineState* pso)
{
	BL_ASSERT(pso);

	D3D12GraphicsPipelineState* d3d12Pso = static_cast<D3D12GraphicsPipelineState*>(pso);

	m_pCommandList->SetPipelineState(d3d12Pso->m_pPSO);
}

void D3D12GraphicsContext::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop)
{
	BL_ASSERT(primTop != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

	m_pCommandList->IASetPrimitiveTopology(primTop);
}

void D3D12GraphicsContext::ResourceBarrier(IGraphicsTexture* graphicsTexture, D3D12_RESOURCE_STATES desiredState, unsigned int subResource)
{
	// Cache some objects and do some sanity checks
	BL_ASSERT(graphicsTexture);
	D3D12GraphicsTexture* d3d12Texture = static_cast<D3D12GraphicsTexture*>(graphicsTexture);

	D3D12GlobalStateTracker* globalStateTracker = d3d12Texture->m_GlobalStateTracker;
	BL_ASSERT(globalStateTracker);

	ID3D12Resource1* resource = d3d12Texture->m_pResource;
	BL_ASSERT(resource);

	// If this resource doesn't have a local state, create one
	D3D12LocalStateTracker* localStateTracker = m_GlobalToLocalMap[globalStateTracker];
	if (localStateTracker == nullptr)
	{
		D3D12LocalStateTracker* localStateTracker = new D3D12LocalStateTracker(resource, d3d12Texture->m_NumMipLevels);
		m_GlobalToLocalMap[globalStateTracker] = localStateTracker;
	}

	// Add resourceBarrier to be resolved later by the transitionCList if current state is unknown
	// If we know the currentState, we can do the transition immediatly on the mainCommandList
	localStateTracker->ResolveLocalResourceState(desiredState, m_PendingResourceBarriers, m_pCommandList, subResource);
}

void D3D12GraphicsContext::ResourceBarrier(IGraphicsBuffer* graphicsBuffer, D3D12_RESOURCE_STATES desiredState, unsigned int subResource)
{
	// Cache some objects and do some sanity checks
	BL_ASSERT(graphicsBuffer);
	D3D12GraphicsBuffer* d3d12Buffer = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer);
	
	D3D12GlobalStateTracker* globalStateTracker = d3d12Buffer->m_GlobalStateTracker;
	BL_ASSERT(globalStateTracker);

	ID3D12Resource1* resource = d3d12Buffer->m_pResource;
	BL_ASSERT(resource);

	// If this resource doesn't have a local state, create one
	D3D12LocalStateTracker* localStateTracker = m_GlobalToLocalMap[globalStateTracker];
	if (localStateTracker == nullptr)
	{
		D3D12LocalStateTracker* localStateTracker = new D3D12LocalStateTracker(resource, 1);
		m_GlobalToLocalMap[globalStateTracker] = localStateTracker;
	}

	// Add resourceBarrier to be resolved later by the transitionCList if current state is unknown
	// If we know the currentState, we can do the transition immediatly on the mainCommandList
	localStateTracker->ResolveLocalResourceState(desiredState, m_PendingResourceBarriers, m_pCommandList, subResource);
}

void D3D12GraphicsContext::UAVBarrier(IGraphicsTexture* graphicsTexture)
{
	BL_ASSERT(graphicsTexture);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = static_cast<D3D12GraphicsTexture*>(graphicsTexture)->m_pResource;
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void D3D12GraphicsContext::UAVBarrier(IGraphicsBuffer* graphicsBuffer)
{
	BL_ASSERT(graphicsBuffer);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer)->m_pResource;
	m_pCommandList->ResourceBarrier(1, &barrier);
}

void D3D12GraphicsContext::SetViewPort(unsigned int width, unsigned int height, float topLeftX, float topLeftY, float minDepth, float maxDepth)
{
	D3D12_VIEWPORT viewPort = {};
	viewPort.Width = width;
	viewPort.Height = height;
	viewPort.TopLeftX = topLeftX;
	viewPort.TopLeftY = topLeftY;
	viewPort.MinDepth = minDepth;
	viewPort.MaxDepth = maxDepth;

	m_pCommandList->RSSetViewports(1, &viewPort);
}

void D3D12GraphicsContext::SetScizzorRect(unsigned int right, unsigned int bottom, float left, unsigned int top)
{
	D3D12_RECT rect = {};
	rect.right = right;
	rect.bottom = bottom;
	rect.left = left;
	rect.top = top;

	m_pCommandList->RSSetScissorRects(1, &rect);
}

void D3D12GraphicsContext::CopyTextureRegion(IGraphicsTexture* dst, IGraphicsTexture* src, unsigned int xStart, unsigned int yStart)
{
	BL_ASSERT(dst);
	BL_ASSERT(src);

	D3D12_TEXTURE_COPY_LOCATION copyDst = {};
	copyDst.pResource = static_cast<D3D12GraphicsTexture*>(dst)->m_pResource;
	copyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	copyDst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION copySrc = {};
	copySrc.pResource = static_cast<D3D12GraphicsTexture*>(src)->m_pResource;
	copySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	copySrc.SubresourceIndex = 0;

	m_pCommandList->CopyTextureRegion(&copyDst, xStart, yStart, 0, &copySrc, nullptr);
}

void D3D12GraphicsContext::ClearDepthTexture(IGraphicsTexture* depthTexture, bool clearDepth, float depthValue, bool clearStencil, unsigned int stencilValue)
{
	BL_ASSERT(depthTexture);

	D3D12GraphicsTexture* d3d12DepthTexture = static_cast<D3D12GraphicsTexture*>(depthTexture);
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = graphicsManager->GetDSVDescriptorHeap()->GetCPUHeapAt(d3d12DepthTexture->GetDepthStencilIndex());

	D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
	clearFlags |= clearDepth ? D3D12_CLEAR_FLAG_DEPTH : (D3D12_CLEAR_FLAGS)0;
	clearFlags |= clearStencil ? D3D12_CLEAR_FLAG_STENCIL : (D3D12_CLEAR_FLAGS)0;

	m_pCommandList->ClearDepthStencilView(cpuHandle, clearFlags, depthValue, stencilValue, 0, nullptr);
}

void D3D12GraphicsContext::ClearRenderTarget(IGraphicsTexture* renderTargetTexture, float clearColor[4])
{
	BL_ASSERT(renderTargetTexture);

	D3D12GraphicsTexture* d3d12RenderTargetTexture = static_cast<D3D12GraphicsTexture*>(renderTargetTexture);
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = graphicsManager->GetRTVDescriptorHeap()->GetCPUHeapAt(d3d12RenderTargetTexture->GetRenderTargetHeapIndex());
	m_pCommandList->ClearRenderTargetView(cpuHandle, clearColor, 0, nullptr);
}

void D3D12GraphicsContext::ClearUAVTextureFloat(IGraphicsTexture* uavTexture, float clearValues[4], unsigned int mipLevel)
{
	BL_ASSERT(uavTexture);
	D3D12GraphicsTexture* d3d12Texture = static_cast<D3D12GraphicsTexture*>(uavTexture);
	BL_ASSERT_MESSAGE(d3d12Texture->m_CPUDescriptorHeap, "Trying to clear a texture which doesn't have UAV properties!\n");

	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	D3D12DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mainDHeap->GetGPUHeapAt(uavTexture->GetUnorderedAccessIndex(mipLevel));
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = d3d12Texture->m_CPUDescriptorHeap->GetCPUHeapAt(mipLevel);
	m_pCommandList->ClearUnorderedAccessViewFloat(gpuHandle, cpuHandle, d3d12Texture->m_pResource, clearValues, 0, nullptr);
}

void D3D12GraphicsContext::ClearUAVTextureUINT(IGraphicsTexture* uavTexture, unsigned int clearValues[4], unsigned int mipLevel)
{
	BL_ASSERT(uavTexture);
	D3D12GraphicsTexture* d3d12Texture = static_cast<D3D12GraphicsTexture*>(uavTexture);
	BL_ASSERT_MESSAGE(d3d12Texture->m_CPUDescriptorHeap, "Trying to clear a texture which doesn't have UAV properties!\n");

	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	D3D12DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mainDHeap->GetGPUHeapAt(uavTexture->GetUnorderedAccessIndex(mipLevel));
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = d3d12Texture->m_CPUDescriptorHeap->GetCPUHeapAt(mipLevel);
	m_pCommandList->ClearUnorderedAccessViewUint(gpuHandle, cpuHandle, d3d12Texture->m_pResource, clearValues, 0, nullptr);
}

void D3D12GraphicsContext::SetRenderTargets(unsigned int numRenderTargets, IGraphicsTexture* renderTargetTextures[], IGraphicsTexture* depthTexture)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuRTVHandles[8] = {};
	for (unsigned int i = 0; i < numRenderTargets; i++)
	{
		unsigned int rtvIndex = static_cast<D3D12GraphicsTexture*>(renderTargetTextures[i])->GetRenderTargetHeapIndex();
		cpuRTVHandles[i] = graphicsManager->GetRTVDescriptorHeap()->GetCPUHeapAt(rtvIndex);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuDSVHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE* pcpuDSVHandle = nullptr;
	if (depthTexture != nullptr)
	{
		unsigned int dsvIndex = static_cast<D3D12GraphicsTexture*>(depthTexture)->GetDepthStencilIndex();
		cpuDSVHandle = graphicsManager->GetDSVDescriptorHeap()->GetCPUHeapAt(dsvIndex);
		pcpuDSVHandle = &cpuDSVHandle;
	}

	m_pCommandList->OMSetRenderTargets(numRenderTargets, cpuRTVHandles, false, pcpuDSVHandle);
}

void D3D12GraphicsContext::SetShaderResourceView(unsigned int rootParamSlot, IGraphicsTexture* graphicsTexture, bool isComputePipeline)
{
	BL_ASSERT(graphicsTexture);

	D3D12GraphicsTexture* d3d12GraphicsTexture = static_cast<D3D12GraphicsTexture*>(graphicsTexture);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootShaderResourceView(rootParamSlot, d3d12GraphicsTexture->m_pResource->GetGPUVirtualAddress());
	}
	else
	{
		m_pCommandList->SetGraphicsRootShaderResourceView(rootParamSlot, d3d12GraphicsTexture->m_pResource->GetGPUVirtualAddress());
	}
}

void D3D12GraphicsContext::SetShaderResourceView(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline)
{
	BL_ASSERT(graphicsBuffer);

	D3D12GraphicsBuffer* d3d12GraphicsBuffer = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootShaderResourceView(rootParamSlot, d3d12GraphicsBuffer->m_pResource->GetGPUVirtualAddress());
	}
	else
	{
		m_pCommandList->SetGraphicsRootShaderResourceView(rootParamSlot, d3d12GraphicsBuffer->m_pResource->GetGPUVirtualAddress());
	}
}

void D3D12GraphicsContext::SetConstantBuffer(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline)
{
	BL_ASSERT(graphicsBuffer);

	D3D12GraphicsBuffer* d3d12GraphicsBuffer = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootConstantBufferView(rootParamSlot, d3d12GraphicsBuffer->m_pResource->GetGPUVirtualAddress());
	}
	else
	{
		m_pCommandList->SetGraphicsRootConstantBufferView(rootParamSlot, d3d12GraphicsBuffer->m_pResource->GetGPUVirtualAddress());
	}
}

void D3D12GraphicsContext::SetConstantBufferDynamicData(unsigned int rootParamSlot, unsigned int size, const void* data, bool isComputePipeline)
{
	BL_ASSERT(size);
	BL_ASSERT(data);

	DynamicDataParams params = D3D12GraphicsManager::GetInstance()->SetDynamicData(size, data);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootConstantBufferView(rootParamSlot, params.vAddr);
	}
	else
	{
		m_pCommandList->SetGraphicsRootConstantBufferView(rootParamSlot, params.vAddr);
	}
}

void D3D12GraphicsContext::Set32BitConstants(unsigned int rootParamSlot, unsigned int num32BitValuesToSet, const void* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline)
{
	BL_ASSERT(pSrcData);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRoot32BitConstants(rootParamSlot, num32BitValuesToSet, pSrcData, offsetIn32BitValues);
	}
	else
	{
		m_pCommandList->SetGraphicsRoot32BitConstants(rootParamSlot, num32BitValuesToSet, pSrcData, offsetIn32BitValues);
	}
}

void D3D12GraphicsContext::SetStencilRef(unsigned int stencilRef)
{
	m_pCommandList->OMSetStencilRef(stencilRef);
}

void D3D12GraphicsContext::SetIndexBuffer(IGraphicsBuffer* indexBuffer, unsigned int sizeInBytes)
{
	BL_ASSERT(indexBuffer);
	BL_ASSERT(sizeInBytes);

	D3D12GraphicsBuffer* d3d12IndexBuffer = static_cast<D3D12GraphicsBuffer*>(indexBuffer);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = d3d12IndexBuffer->m_pResource->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = sizeInBytes;

	m_pCommandList->IASetIndexBuffer(&indexBufferView);
}

void D3D12GraphicsContext::DrawIndexedInstanced(unsigned int indexCountPerInstance, unsigned int instanceCount, unsigned int startIndexLocation, int baseVertexLocation, unsigned int startInstanceLocation)
{
	BL_ASSERT(indexCountPerInstance);
	BL_ASSERT(instanceCount);

	m_pCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void D3D12GraphicsContext::Dispatch(unsigned int threadGroupsX, unsigned int threadGroupsY, unsigned int threadGroupsZ)
{
	BL_ASSERT(threadGroupsX != 0 || threadGroupsY != 0 || threadGroupsZ != 0);

	m_pCommandList->Dispatch(threadGroupsX, threadGroupsY, threadGroupsZ);
}

void D3D12GraphicsContext::DrawImGui()
{
	// Good to have
	// bool a = true;
	// ImGui::ShowDemoWindow(&a);

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	BL_ASSERT(drawData);

	ImGui_ImplDX12_RenderDrawData(drawData, m_pCommandList);
}

void D3D12GraphicsContext::BuildTLAS(ITopLevelAS* pTlas)
{
	BL_ASSERT(pTlas);

	D3D12TopLevelAS* d3d12Tlas = static_cast<D3D12TopLevelAS*>(pTlas);
	m_pCommandList->BuildRaytracingAccelerationStructure(&d3d12Tlas->m_BuildDesc, 0, nullptr);

	this->UAVBarrier(d3d12Tlas->m_pResultBuffer);
}

void D3D12GraphicsContext::BuildBLAS(IBottomLevelAS* pBlas)
{
	BL_ASSERT(pBlas);

	D3D12BottomLevelAS* d3d12Blas = static_cast<D3D12BottomLevelAS*>(pBlas);
	m_pCommandList->BuildRaytracingAccelerationStructure(&d3d12Blas->m_BuildDesc, 0, nullptr);

	this->UAVBarrier(d3d12Blas->m_pResultBuffer);
}

void D3D12GraphicsContext::DispatchRays(IShaderBindingTable* sbt, unsigned int dispatchWidth, unsigned int dispatchHeight, unsigned int dispatchDepth)
{
	BL_ASSERT(sbt);
	BL_ASSERT(dispatchWidth || dispatchHeight || dispatchWidth); // All entries gotta be atleast 1!

	D3D12GraphicsManager* d3d12Manager = D3D12GraphicsManager::GetInstance();
	D3D12ShaderBindingTable* d3d12SBT = static_cast<D3D12ShaderBindingTable*>(sbt);
	D3D12GraphicsBuffer* d3d12SbtBuffer = static_cast<D3D12GraphicsBuffer*>(d3d12SBT->m_pSBTBuffer[d3d12Manager->GetCommandInterfaceIndex()]);
	BL_ASSERT(d3d12SbtBuffer);

	D3D12_GPU_VIRTUAL_ADDRESS vAddr = d3d12SbtBuffer->m_pResource->GetGPUVirtualAddress();

	// Setup the raytracing task
	D3D12_DISPATCH_RAYS_DESC desc = {};

	// Ray generation section
	uint32_t rayGenerationSectionSizeInBytes = d3d12SBT->GetRayGenSectionSize();
	desc.RayGenerationShaderRecord.StartAddress = vAddr;
	desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

	// Miss section
	uint32_t missSectionSizeInBytes = d3d12SBT->GetMissSectionSize();
	desc.MissShaderTable.StartAddress = vAddr + rayGenerationSectionSizeInBytes;
	desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
	desc.MissShaderTable.StrideInBytes = d3d12SBT->GetMaxMissShaderRecordSize();

	// Hit group section
	uint32_t hitGroupsSectionSize = d3d12SBT->GetHitGroupSectionSize();
	desc.HitGroupTable.StartAddress = vAddr +
		rayGenerationSectionSizeInBytes +
		missSectionSizeInBytes;
	desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
	desc.HitGroupTable.StrideInBytes = d3d12SBT->GetMaxHitGroupShaderRecordSize();

	// Dimensions of the image to render
	desc.Width	= dispatchWidth;
	desc.Height = dispatchHeight;
	desc.Depth	= dispatchDepth;

	m_pCommandList->DispatchRays(&desc);
}

void D3D12GraphicsContext::SetRayTracingPipelineState(IRayTracingPipelineState* rtPipelineState)
{
	BL_ASSERT(rtPipelineState);

	D3D12RayTracingPipelineState* d3d12rtState = static_cast<D3D12RayTracingPipelineState*>(rtPipelineState);
	m_pCommandList->SetPipelineState1(d3d12rtState->m_pRayTracingStateObject);
}

void D3D12GraphicsContext::resolvePendingTransitionBarriers()
{
	// This function has to be called on mainThread

	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	unsigned int index = graphicsManager->GetCommandInterfaceIndex();

	// Reset TransitionCommandinterface (they are closed by the mainThread when the transitions have been added)
	D3D12GraphicsManager::CHECK_HRESULT(m_pTransitionCommandAllocators[index]->Reset());
	D3D12GraphicsManager::CHECK_HRESULT(m_pTransitionCommandList->Reset(m_pTransitionCommandAllocators[index], NULL));

	TODO("Resolve pending barriers");
	{

	}

	// End
	D3D12GraphicsManager::CHECK_HRESULT(m_pTransitionCommandList->Close());
}
