#include "stdafx.h"
#include "ShadowRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../GPUMemory/GPUMemory.h"
#include "../DescriptorHeap.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../RootSignature.h"

// Techniques
#include "../Techniques/ShadowInfo.h"

// Model info
#include "../Renderer/Model/Transform.h"
#include "../Renderer/Model/Mesh.h"

ShadowRenderTask::ShadowRenderTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{

}

ShadowRenderTask::~ShadowRenderTask()
{
}

void ShadowRenderTask::AddShadowCastingLight(std::pair<Light*, ShadowInfo*> light)
{
	m_lights.push_back(light);
}

void ShadowRenderTask::ClearSpecificLight(Light* light)
{
	unsigned int i = 0;
	for (auto& pair : m_lights)
	{
		if (pair.first == light)
		{
			m_lights.erase(m_lights.begin() + i);
		}
		i++;
	}
}

void ShadowRenderTask::Clear()
{
	m_lights.clear();
}

void ShadowRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);
	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	
	commandList->SetGraphicsRootSignature(m_pRootSig);

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	commandList->SetGraphicsRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::DSV];

	// Draw for every shadow-casting-light
	for (auto light : m_lights)
	{
		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		const D3D12_VIEWPORT* viewPort = light.second->GetRenderView()->GetViewPort();
		const D3D12_RECT* rect = light.second->GetRenderView()->GetScissorRect();
		commandList->RSSetViewports(1, viewPort);
		commandList->RSSetScissorRects(1, rect);

		const DirectX::XMMATRIX* viewProjMatTrans = light.first->GetCamera()->GetViewProjectionTranposed();

		light.second->GetResource()->TransResourceState(
			commandList,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);

		// Clear and set depthstencil
		unsigned int dsvIndex = light.second->GetDSV()->GetDescriptorHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(dsvIndex);
		commandList->ClearDepthStencilView(dsh, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList->OMSetRenderTargets(0, nullptr, true, &dsh);

		// Draw for every Rendercomponent
		for (int i = 0; i < m_RenderComponents.size(); i++)
		{
			component::ModelComponent* mc = m_RenderComponents.at(i).first;
			component::TransformComponent* tc = m_RenderComponents.at(i).second;

			// Draw for every m_pMesh the meshComponent has
			for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
			{
				unsigned int num_Indices = mc->GetMeshAt(i)->GetNumIndices();
				const SlotInfo* info = mc->GetSlotInfoAt(i);

				Transform* transform = tc->GetTransform();
				DirectX::XMMATRIX* WTransposed = transform->GetWorldMatrixTransposed();
				DirectX::XMMATRIX WVPTransposed = (*viewProjMatTrans) * (*WTransposed);

				// Create a CB_PER_OBJECT struct
				CB_PER_OBJECT_STRUCT perObject = { *WTransposed, WVPTransposed, *info };

				commandList->SetGraphicsRoot32BitConstants(RS::CB_PER_OBJECT_CONSTANTS, sizeof(CB_PER_OBJECT_STRUCT) / sizeof(UINT), &perObject, 0);

				commandList->IASetIndexBuffer(mc->GetMeshAt(i)->GetIndexBufferView());
				commandList->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
			}
		}

		light.second->GetResource()->TransResourceState(
			commandList,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	commandList->Close();
}
