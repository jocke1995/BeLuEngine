#include "stdafx.h"
#include "WireframeRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../PipelineState/PipelineState.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
#include "../API/D3D12/D3D12GraphicsBuffer.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"

WireframeRenderTask::WireframeRenderTask()
	:GraphicsPass(L"BoundingBoxPass")
{
}

WireframeRenderTask::~WireframeRenderTask()
{

}

void WireframeRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(WireFramePass, commandList);

		commandList->SetGraphicsRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		DescriptorHeap* renderTargetHeap = rtvHeap;

		unsigned int renderTargetIndex = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["finalColorBuffer"])->GetRenderTargetHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(renderTargetIndex);

		commandList->OMSetRenderTargets(1, &cdh, false, nullptr);

		TODO("Fix the sizes");
		D3D12_VIEWPORT viewPort = {};
		viewPort.TopLeftX = 0.0f;
		viewPort.TopLeftY = 0.0f;
		viewPort.Width = 1280;
		viewPort.Height = 720;
		viewPort.MinDepth = 0.0f;
		viewPort.MaxDepth = 1.0f;

		D3D12_RECT rect = {};
		rect.left = 0;
		rect.right = 1280;
		rect.top = 0;
		rect.bottom = 720;

		commandList->RSSetViewports(1, &viewPort);
		commandList->RSSetScissorRects(1, &rect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());

		const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

		// Draw for every mesh
		for (int i = 0; i < m_ObjectsToDraw.size(); i++)
		{
			for (int j = 0; j < m_ObjectsToDraw[i]->GetNumBoundingBoxes(); j++)
			{
				const Mesh* m = m_ObjectsToDraw[i]->GetMeshAt(j);
				Transform* t = m_ObjectsToDraw[i]->GetTransformAt(j);

				unsigned int num_Indices = m->GetNumIndices();
				const SlotInfo* info = m_ObjectsToDraw[i]->GetSlotInfo(j);

				commandList->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), info, 0);
				commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B2, static_cast<D3D12GraphicsBuffer*>(t->m_pConstantBuffer)->GetTempResource()->GetGPUVirtualAddress());

				D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
				indexBufferView.BufferLocation = static_cast<D3D12GraphicsBuffer*>(m->GetIndexBuffer())->GetTempResource()->GetGPUVirtualAddress();
				indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				indexBufferView.SizeInBytes = m->GetSizeOfIndices();
				commandList->IASetIndexBuffer(&indexBufferView);

				commandList->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
			}
		}
	}
	commandList->Close();
}

void WireframeRenderTask::AddObjectToDraw(component::BoundingBoxComponent* bbc)
{
	m_ObjectsToDraw.push_back(bbc);
}

void WireframeRenderTask::Clear()
{
	m_ObjectsToDraw.clear();
}

void WireframeRenderTask::ClearSpecific(component::BoundingBoxComponent* bbc)
{
	unsigned int i = 0;
	for (auto& bbcInTask : m_ObjectsToDraw)
	{
		if (bbcInTask == bbc)
		{
			m_ObjectsToDraw.erase(m_ObjectsToDraw.begin() + i);
			break;
		}
		i++;
	}
}
