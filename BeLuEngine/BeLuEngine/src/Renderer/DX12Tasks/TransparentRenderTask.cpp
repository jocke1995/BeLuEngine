#include "stdafx.h"
#include "TransparentRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../RootSignature.h"

// Model info
#include "../Renderer/Model/Transform.h"
#include "../Renderer/Model/Mesh.h"
#include "../Renderer/Model/Material.h"

TransparentRenderTask::TransparentRenderTask(	
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
}

TransparentRenderTask::~TransparentRenderTask()
{

}

void TransparentRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);
	const RenderTargetView* mainColorTargetRenderTarget = m_RenderTargetViews["mainColorTarget"];
	ID3D12Resource1* mainColorTargetResource = mainColorTargetRenderTarget->GetResource()->GetID3D12Resource1();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	commandList->SetGraphicsRootSignature(m_pRootSig);
	
	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	commandList->SetGraphicsRootDescriptorTable(RS::dtCBV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
	commandList->SetGraphicsRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::RTV];
	DescriptorHeap* depthBufferHeap  = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::DSV];

	unsigned int renderTargetIndex = mainColorTargetRenderTarget->GetDescriptorHeapIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(renderTargetIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex());

	commandList->OMSetRenderTargets(1, &cdh, true, &dsh);

	const D3D12_VIEWPORT* viewPort = mainColorTargetRenderTarget->GetRenderView()->GetViewPort();
	const D3D12_RECT* rect = mainColorTargetRenderTarget->GetRenderView()->GetScissorRect();
	commandList->RSSetViewports(1, viewPort);
	commandList->RSSetScissorRects(1, rect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create a CB_PER_FRAME struct
	CB_PER_FRAME_STRUCT perFrame = { m_pCamera->GetPosition().x, m_pCamera->GetPosition().y, m_pCamera->GetPosition().z };
	commandList->SetGraphicsRootConstantBufferView(RS::CB_PER_FRAME, m_Resources["cbPerFrame"]->GetGPUVirtualAdress());
	commandList->SetGraphicsRootConstantBufferView(RS::CB_PER_SCENE, m_Resources["cbPerScene"]->GetGPUVirtualAdress());
	commandList->SetGraphicsRootShaderResourceView(RS::RAWBUFFER_LIGHTS, m_Resources["rawBufferLights"]->GetGPUVirtualAdress());

	const DirectX::XMMATRIX * viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

	// Draw from opposite order from the sorted array
	for(int i = m_RenderComponents.size() - 1; i >= 0; i--)
	{
		component::ModelComponent* mc = m_RenderComponents.at(i).mc;
		component::TransformComponent* tc = m_RenderComponents.at(i).tc;

		// Draw for every m_pMesh the MeshComponent has
		for (unsigned int j = 0; j < mc->GetNrOfMeshes(); j++)
		{
			Mesh* m = mc->GetMeshAt(j);
			unsigned int num_Indices = m->GetNumIndices();
			const SlotInfo* info = mc->GetSlotInfoAt(j);

			Transform* t = tc->GetTransform();

			commandList->SetGraphicsRootConstantBufferView(RS::CBV0, mc->GetMaterialAt(i)->GetMaterialData()->first->GetDefaultResource()->GetGPUVirtualAdress());
			commandList->SetGraphicsRoot32BitConstants(RS::SLOTINFO_CONSTANTS, sizeof(SlotInfo) / sizeof(UINT), info, 0);
			commandList->SetGraphicsRootConstantBufferView(RS::MATRICES_PER_OBJECT_CBV, t->m_pCB->GetDefaultResource()->GetGPUVirtualAdress());

			commandList->IASetIndexBuffer(mc->GetMeshAt(j)->GetIndexBufferView());
			// Draw each object twice with different PSO 
			for (int k = 0; k < 2; k++)
			{
				commandList->SetPipelineState(m_PipelineStates[k]->GetPSO());
				commandList->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
			}
		}
	}

	commandList->Close();
}
