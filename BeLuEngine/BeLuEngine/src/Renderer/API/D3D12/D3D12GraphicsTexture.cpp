#include "stdafx.h"
#include "D3D12GraphicsTexture.h"

TODO("This should be inside stdafx.h");
#include "../Misc/Log.h"

#include "External/TextureFunctions.h"

#include "D3D12GraphicsManager.h"

#include "../Renderer/DescriptorHeap.h"

TODO("Wrapper for DXGI_FORMAT");

D3D12GraphicsTexture::D3D12GraphicsTexture()
	:IGraphicsTexture()
{
	
}

D3D12GraphicsTexture::~D3D12GraphicsTexture()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	BL_SAFE_RELEASE(&m_pResource);
}

bool D3D12GraphicsTexture::CreateTexture2D(const std::wstring& filePath, DXGI_FORMAT dxgiFormat, F_TEXTURE_USAGE textureUsage)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();
	DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();

	HRESULT hr;

	// Read from file
	if (textureUsage & F_TEXTURE_USAGE::Normal)
	{
#pragma region ReadFromFile
		// DDSLoader uses this data type to load the image data
		// converts this to m_pImageData when it is used.
		std::unique_ptr<uint8_t[]> m_DdsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;
		// Loads the texture and creates a default resource;
		hr = DirectX::LoadDDSTextureFromFile(device5, filePath.c_str(), reinterpret_cast<ID3D12Resource**>(&m_pResource), m_DdsData, subresourceData);

		if (!D3D12GraphicsManager::SucceededHRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to create texture: \'%s\'.\n", to_string(filePath).c_str());
			BL_SAFE_RELEASE(&m_pResource);
			return false;
		}

		std::wstring resourceName = filePath + L"_DEFAULT_RESOURCE";
		graphicsManager->SucceededHRESULT(m_pResource->SetName(resourceName.c_str()));

		// Set resource desc created in LoadDDSTextureFromFile
		D3D12_RESOURCE_DESC resourceDesc = m_pResource->GetDesc();
		unsigned int rowPitch = subresourceData[0].RowPitch;

		// copy m_DdsData to our BYTE* format
		unsigned char* imageData = static_cast<BYTE*>(m_DdsData.get());
		m_DdsData.release(); // lose the pointer, let m_pImageData delete the data.

		// Footprint
		unsigned __int64 totalSizeInBytes = 0;

		device5->GetCopyableFootprints(
			&resourceDesc,
			0, resourceDesc.MipLevels, 0,
			nullptr, nullptr, nullptr,
			&totalSizeInBytes);

#pragma endregion

#pragma region CreateBuffer
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.CreationNodeMask = 1; //used when multi-gpu
		heapProps.VisibleNodeMask = 1; //used when multi-gpu
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = totalSizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		hr = graphicsManager->GetDevice()->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&m_pResource)
		);

		resourceName = filePath + L"_UPLOAD_RESOURCE";

		if (!graphicsManager->SucceededHRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to create D3D12GraphicsTexture with name: \'%s\'\n", resourceName.c_str());
			return false;
		}

		graphicsManager->SucceededHRESULT(m_pResource->SetName(resourceName.c_str()));
#pragma endregion
	}
	else
	{
		TODO("Fix");
	}
	


	

	// Create srv
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = m_ResourceDescription.Format;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = m_pDefaultResource->GetID3D12Resource1()->GetDesc().MipLevels;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;

	m_pSRV = new ShaderResourceView(
		device,
		descriptorHeap,
		&desc,
		m_pDefaultResource);

	CoInitialize(NULL);
}
