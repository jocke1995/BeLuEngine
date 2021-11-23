#include "stdafx.h"
#include "OutliningRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/GraphicsState.h"
#include "../RenderView.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

OutliningRenderTask::OutliningRenderTask(
	ID3D12Device5* device,
	ID3D12RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	DescriptorHeap* cbvHeap,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	// Init with nullptr
	Clear();

	m_OutlineTransformToScale.m_pCB = new ConstantBuffer(device, sizeof(DirectX::XMMATRIX) * 2, L"OutlinedTransform", cbvHeap);
}

OutliningRenderTask::~OutliningRenderTask()
{
	delete m_OutlineTransformToScale.m_pCB;
}

void OutliningRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);
	const RenderTargetView* gBufferAlbedoRenderTarget = m_RenderTargetViews["finalColorBuffer"];
	ID3D12Resource1* gBufferAlbedoResource = gBufferAlbedoRenderTarget->GetResource()->GetID3D12Resource1();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(OutliningPass, commandList);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::RTV];
		DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::DSV];

		const unsigned int gBufferAlbedoIndex = gBufferAlbedoRenderTarget->GetDescriptorHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(gBufferAlbedoIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex());

		// Check if there is an object to outline
		if (m_ObjectToOutline.first == nullptr)
		{
			commandList->ClearDepthStencilView(dsh, D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
			goto End;	// Hack
		}
		// else continue as usual

		commandList->SetGraphicsRootSignature(m_pRootSig);
		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		commandList->OMSetRenderTargets(1, &cdh, true, &dsh);

		const D3D12_VIEWPORT* viewPort = gBufferAlbedoRenderTarget->GetRenderView()->GetViewPort();
		const D3D12_RECT* rect = gBufferAlbedoRenderTarget->GetRenderView()->GetScissorRect();
		commandList->RSSetViewports(1, viewPort);
		commandList->RSSetScissorRects(1, rect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());

		const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

		// Draw for every m_pMesh
		for (int i = 0; i < m_ObjectToOutline.first->GetNrOfMeshes(); i++)
		{
			Mesh* m = m_ObjectToOutline.first->GetMeshAt(i);
			Transform* t = m_ObjectToOutline.second->GetTransform();
			Transform newScaledTransform = *t;
			newScaledTransform.IncreaseScaleByPercent(0.02f);
			newScaledTransform.UpdateWorldMatrix();

			component::ModelComponent* mc = m_ObjectToOutline.first;

			unsigned int num_Indices = m->GetNumIndices();
			const SlotInfo* info = mc->GetSlotInfoAt(i);

			DirectX::XMMATRIX w_wvp[2] = {};
			w_wvp[0] = *newScaledTransform.GetWorldMatrixTransposed();
			w_wvp[1] = (*viewProjMatTrans) * w_wvp[0];

			m_OutlineTransformToScale.m_pCB->GetUploadResource()->SetData(&w_wvp);
			commandList->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), info, 0);
			commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B2, m_OutlineTransformToScale.m_pCB->GetUploadResource()->GetGPUVirtualAdress());

			commandList->IASetIndexBuffer(m->GetIndexBufferView());

			commandList->OMSetStencilRef(1);
			commandList->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
		}
	}
End:
	commandList->Close();
}

void OutliningRenderTask::SetObjectToOutline(std::pair<component::ModelComponent*, component::TransformComponent*>* objectToOutline)
{
	m_ObjectToOutline = *objectToOutline;
}

void OutliningRenderTask::Clear()
{
	m_ObjectToOutline.first = nullptr;
	m_ObjectToOutline.second = nullptr;
}
