#include "stdafx.h"
#include "TransparentRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../PipelineState/PipelineState.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
#include "../API/D3D12/D3D12GraphicsBuffer.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"

TransparentRenderTask::TransparentRenderTask(	
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
}

TransparentRenderTask::~TransparentRenderTask()
{

}

void TransparentRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(TransparentPass, commandList);

		commandList->SetGraphicsRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(dtCBV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		DescriptorHeap* renderTargetHeap = rtvHeap;
		DescriptorHeap* depthBufferHeap = dsvHeap;

		unsigned int rtvIndex = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["finalColorBuffer"])->GetRenderTargetHeapIndex();
		unsigned int dsvIndex = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["mainDepthStencilBuffer"])->GetDepthStencilIndex();

		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(rtvIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(dsvIndex);

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

		// Create a CB_PER_FRAME struct
		CB_PER_FRAME_STRUCT perFrame = { m_pCamera->GetPosition().x, m_pCamera->GetPosition().y, m_pCamera->GetPosition().z };
		commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B3, static_cast<D3D12GraphicsBuffer*>(m_GraphicBuffers["cbPerFrame"])->GetTempResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B4, static_cast<D3D12GraphicsBuffer*>(m_GraphicBuffers["cbPerScene"])->GetTempResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootShaderResourceView(RootParam_SRV_T0, static_cast<D3D12GraphicsBuffer*>(m_GraphicBuffers["rawBufferLights"])->GetTempResource()->GetGPUVirtualAddress());

		const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

		// Draw from opposite order from the sorted array
		for (int i = m_RenderComponents.size() - 1; i >= 0; i--)
		{
			component::ModelComponent* mc = m_RenderComponents.at(i).mc;
			component::TransformComponent* tc = m_RenderComponents.at(i).tc;

			// Draw for every m_pMesh the MeshComponent has
			commandList->SetGraphicsRootShaderResourceView(RootParam_SRV_T1, static_cast<D3D12GraphicsBuffer*>(mc->GetMaterialByteAdressBuffer())->GetTempResource()->GetGPUVirtualAddress());			for (unsigned int j = 0; j < mc->GetNrOfMeshes(); j++)
			{
				Mesh* m = mc->GetMeshAt(j);
				unsigned int num_Indices = m->GetNumIndices();
				const SlotInfo* info = mc->GetSlotInfoAt(j);

				Transform* t = tc->GetTransform();

				commandList->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), info, 0);
				commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B2, static_cast<D3D12GraphicsBuffer*>(t->m_pConstantBuffer)->GetTempResource()->GetGPUVirtualAddress());

				D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
				indexBufferView.BufferLocation = static_cast<D3D12GraphicsBuffer*>(mc->GetMeshAt(j)->GetIndexBuffer())->GetTempResource()->GetGPUVirtualAddress();
				indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				indexBufferView.SizeInBytes = mc->GetMeshAt(j)->GetSizeOfIndices();
				commandList->IASetIndexBuffer(&indexBufferView);

				// Draw each object twice with different PSO 
				for (int k = 0; k < 2; k++)
				{
					commandList->SetPipelineState(m_PipelineStates[k]->GetPSO());
					commandList->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
				}
			}
		}
	}
	commandList->Close();
}
