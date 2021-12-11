#include "stdafx.h"
#include "OutliningRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../PipelineState/GraphicsState.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
#include "../API/D3D12/D3D12GraphicsBuffer.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"


OutliningRenderTask::OutliningRenderTask(
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	// Init with nullptr
	Clear();

	m_OutlineTransformToScale.m_pConstantBuffer = IGraphicsBuffer::Create(
		E_GRAPHICSBUFFER_TYPE::ConstantBuffer,
		sizeof(DirectX::XMMATRIX), 1,
		DXGI_FORMAT_UNKNOWN, L"OutlinedTransform");
}

OutliningRenderTask::~OutliningRenderTask()
{
	delete m_OutlineTransformToScale.m_pConstantBuffer;
}

void OutliningRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();

	ID3D12Resource1* gBufferAlbedoResource = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["finalColorBuffer"])->GetTempResource();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(OutliningPass, commandList);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		DescriptorHeap* renderTargetHeap = rtvHeap;
		DescriptorHeap* depthBufferHeap = dsvHeap;

		const unsigned int gBufferAlbedoIndex = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["finalColorBuffer"])->GetRenderTargetHeapIndex();
		const unsigned int dsvIndex = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["mainDepthStencilBuffer"])->GetDepthStencilIndex();

		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(gBufferAlbedoIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(dsvIndex);

		// Check if there is an object to outline
		if (m_ObjectToOutline.first == nullptr)
		{
			commandList->ClearDepthStencilView(dsh, D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
			goto End;	// Hack
		}
		// else continue as usual

		commandList->SetGraphicsRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig);
		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		commandList->OMSetRenderTargets(1, &cdh, true, &dsh);

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

			DynamicDataParams params = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->SetDynamicData(sizeof(DirectX::XMMATRIX) * 2, &w_wvp);

			commandList->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), info, 0);
			commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B2, params.vAddr);

			D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
			indexBufferView.BufferLocation = static_cast<D3D12GraphicsBuffer*>(m->GetIndexBuffer())->GetTempResource()->GetGPUVirtualAddress();
			indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			indexBufferView.SizeInBytes = m->GetSizeOfIndices();
			commandList->IASetIndexBuffer(&indexBufferView);

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
