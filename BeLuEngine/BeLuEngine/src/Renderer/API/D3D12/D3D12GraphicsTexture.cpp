#include "stdafx.h"
#include "D3D12GraphicsTexture.h"

#include "External/TextureFunctions.h"

#include "D3D12GraphicsManager.h"
#include "D3D12DescriptorHeap.h"

DXGI_FORMAT ConvertFormatToSRGB(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
	case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
	case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
	case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
	default: BL_LOG_WARNING("Cannot convert textureFormat to SRGB"); return format;
	}

	// Should never reach this point
	BL_ASSERT(false);
	return format;
}

D3D12GraphicsTexture::D3D12GraphicsTexture()
	:IGraphicsTexture()
{
	
}

D3D12GraphicsTexture::~D3D12GraphicsTexture()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	graphicsManager->AddD3D12ObjectToDefferedDeletion(m_pResource);

	BL_SAFE_DELETE(m_CPUDescriptorHeap);

	TODO("Fix this properly");
	BL_SAFE_DELETE(m_pTextureData);
}

bool D3D12GraphicsTexture::LoadTextureDDS(E_TEXTURE2D_TYPE textureType, const std::wstring& filePath)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();
	D3D12DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();

	HRESULT hr;

	m_Path = filePath;
#pragma region ReadFromFile
	// DDSLoader uses this data type to load the image data
	// converts this to m_pImageData when it is used.
	std::unique_ptr<uint8_t[]> m_DdsData;
	// Loads the texture and creates a default resource;
	hr = DirectX::LoadDDSTextureFromFile(device5, m_Path.c_str(), reinterpret_cast<ID3D12Resource**>(&m_pResource), m_DdsData, m_Subresources);

	if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to create texture: \'%s\'.\n", to_string(filePath).c_str());
		BL_SAFE_RELEASE(&m_pResource);
		return false;
	}

	std::wstring resourceName = m_Path + L"_DEFAULT_RESOURCE";
	graphicsManager->CHECK_HRESULT(m_pResource->SetName(resourceName.c_str()));

	// Set resource desc created in LoadDDSTextureFromFile
	D3D12_RESOURCE_DESC resourceDesc = m_pResource->GetDesc();
	unsigned int rowPitch = m_Subresources[0].RowPitch;

	m_NumMipLevels = resourceDesc.MipLevels;

	// copy m_DdsData to our BYTE* format
	m_pTextureData = static_cast<BYTE*>(m_DdsData.get());
	m_DdsData.release(); // lose the pointer, let m_pTextureData delete the data.

	device5->GetCopyableFootprints(
		&resourceDesc,
		0, m_NumMipLevels, 0,
		nullptr, nullptr, nullptr,
		&m_Size);

#pragma endregion

	DXGI_FORMAT textureFormat = resourceDesc.Format;
	if (textureType == E_TEXTURE2D_TYPE::ALBEDO)
		textureFormat = ConvertFormatToSRGB(textureFormat);

#pragma region CreateDescriptors
	// Create srv
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = textureFormat;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = m_NumMipLevels;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;

	m_ShaderResourceDescriptorHeapIndices[0] = mainDHeap->GetNextDescriptorHeapIndex(1);
	device5->CreateShaderResourceView(m_pResource, &desc, mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndices[0]));
#pragma endregion

	CoInitialize(NULL);

	return true;
}

bool D3D12GraphicsTexture::CreateTexture2D(unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat, unsigned int textureUsage /* F_TEXTURE_USAGE */, const std::wstring name, D3D12_RESOURCE_STATES startStateTemp, unsigned int mipLevels)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();
	D3D12DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();
	D3D12DescriptorHeap* rtvDHeap = graphicsManager->GetRTVDescriptorHeap();
	D3D12DescriptorHeap* dsvDHeap = graphicsManager->GetDSVDescriptorHeap();

	HRESULT hr;

	m_Path = name;
	m_NumMipLevels = mipLevels;
#pragma region CreateTextureBuffer
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

	if (textureUsage & F_TEXTURE_USAGE::RenderTarget)
		flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	if (textureUsage & F_TEXTURE_USAGE::UnorderedAccess)
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	if (textureUsage & F_TEXTURE_USAGE::DepthStencil)
		flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Format = dxgiFormat;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = m_NumMipLevels;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Flags = flags;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1; //used when multi-gpu
	heapProps.VisibleNodeMask = 1; //used when multi-gpu
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE* pClearValue = nullptr;
	D3D12_CLEAR_VALUE clearValue = {};

	if (textureUsage & F_TEXTURE_USAGE::DepthStencil)
	{
		clearValue.Format = resourceDesc.Format;
		clearValue.DepthStencil.Depth = 1;
		clearValue.DepthStencil.Stencil = 0;

		pClearValue = &clearValue;
	}

	if (textureUsage & F_TEXTURE_USAGE::RenderTarget)
	{
		clearValue.Format = resourceDesc.Format;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		pClearValue = &clearValue;
	}
	

	hr = device5->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		startStateTemp,
		pClearValue,
		IID_PPV_ARGS(&m_pResource)
	);

	BL_ASSERT_MESSAGE(graphicsManager->CHECK_HRESULT(hr), "Failed to create Resource with name: \'%s\'\n", name.c_str());


	hr = m_pResource->SetName(name.c_str());
	graphicsManager->CHECK_HRESULT(hr);

#pragma endregion

#pragma region CreateDescriptors
	if (textureUsage & F_TEXTURE_USAGE::ShaderResource)
	{
		TODO("Fix format wrapper");
		DXGI_FORMAT dxgiFormatTemp = dxgiFormat;
		if (dxgiFormat == DXGI_FORMAT_D24_UNORM_S8_UINT)
			dxgiFormatTemp = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		for (unsigned int i = 0; i < m_NumMipLevels; i++)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = dxgiFormatTemp;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = i;

			m_ShaderResourceDescriptorHeapIndices[i] = mainDHeap->GetNextDescriptorHeapIndex(1);
			D3D12_CPU_DESCRIPTOR_HANDLE cdh = mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndices[i]);

			device5->CreateShaderResourceView(m_pResource, &srvDesc, cdh);
		}
	}

	if (textureUsage & F_TEXTURE_USAGE::RenderTarget)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = dxgiFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		m_RenderTargetDescriptorHeapIndex = rtvDHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = rtvDHeap->GetCPUHeapAt(m_RenderTargetDescriptorHeapIndex);

		device5->CreateRenderTargetView(m_pResource, &rtvDesc, cdh);
	}

	if (textureUsage & F_TEXTURE_USAGE::UnorderedAccess)
	{
		// Create Non-CPUVisible DescriptorHeap and descriptor
		std::wstring dHeapName = name + L"_CPU_DescriptorHeap";
		m_CPUDescriptorHeap = new D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_NumMipLevels, dHeapName, false);

		// Create CPU-Visible descriptors
		for (unsigned int i = 0; i < m_NumMipLevels; i++)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = dxgiFormat;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = i;

			// Main descriptorHeap, used to access the descriptor within shaders
			m_UnorderedAccessDescriptorHeapIndices[i] = mainDHeap->GetNextDescriptorHeapIndex(1);
			D3D12_CPU_DESCRIPTOR_HANDLE cdhMainHeap = mainDHeap->GetCPUHeapAt(m_UnorderedAccessDescriptorHeapIndices[i]);
			device5->CreateUnorderedAccessView(m_pResource, nullptr, &uavDesc, cdhMainHeap);

			// CPUHeap, needed to clear UAV's. This heap is per-texture which has the UAV flag
			// ID3D12GraphicsCommandList5::ClearUnorderedAccessViewFloat
			// ID3D12GraphicsCommandList5::ClearUnorderedAccessViewUint
			D3D12_CPU_DESCRIPTOR_HANDLE cdhCPUHeap = m_CPUDescriptorHeap->GetCPUHeapAt(i);
			device5->CreateUnorderedAccessView(m_pResource, nullptr, &uavDesc, cdhCPUHeap);
		}
	}

	if (textureUsage & F_TEXTURE_USAGE::DepthStencil)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D.MipSlice = 0;

		m_DepthStencilDescriptorHeapIndex = dsvDHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = dsvDHeap->GetCPUHeapAt(m_DepthStencilDescriptorHeapIndex);

		device5->CreateDepthStencilView(m_pResource, &dsvDesc, cdh);
	}

#pragma endregion

	return true;
}

unsigned int D3D12GraphicsTexture::GetShaderResourceHeapIndex(unsigned int mipLevel) const
{
	BL_ASSERT(mipLevel >= 0 && mipLevel < g_MAX_TEXTURE_MIPS);
	BL_ASSERT(m_ShaderResourceDescriptorHeapIndices[mipLevel] != -1);
	return m_ShaderResourceDescriptorHeapIndices[mipLevel];
}

unsigned int D3D12GraphicsTexture::GetUnorderedAccessIndex(unsigned int mipLevel) const
{
	BL_ASSERT(mipLevel >= 0 && mipLevel < g_MAX_TEXTURE_MIPS);
	BL_ASSERT(m_UnorderedAccessDescriptorHeapIndices[mipLevel] != -1);
	return m_UnorderedAccessDescriptorHeapIndices[mipLevel];
}

unsigned int D3D12GraphicsTexture::GetRenderTargetHeapIndex() const
{
	BL_ASSERT(m_RenderTargetDescriptorHeapIndex != -1);
	return m_RenderTargetDescriptorHeapIndex;
}

unsigned int D3D12GraphicsTexture::GetDepthStencilIndex() const
{
	BL_ASSERT(m_DepthStencilDescriptorHeapIndex != -1);
	return m_DepthStencilDescriptorHeapIndex;
}

unsigned __int64 D3D12GraphicsTexture::GetSize() const
{
	BL_ASSERT(m_Size);
	return m_Size;
}
