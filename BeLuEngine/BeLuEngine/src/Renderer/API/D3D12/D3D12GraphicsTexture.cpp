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

	TODO("Fix this properly");
	BL_SAFE_DELETE(m_pTextureData);
}

bool D3D12GraphicsTexture::LoadTextureDDS(const std::wstring& filePath)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();
	DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();

	HRESULT hr;

	m_Path = filePath;
#pragma region ReadFromFile
	// DDSLoader uses this data type to load the image data
	// converts this to m_pImageData when it is used.
	std::unique_ptr<uint8_t[]> m_DdsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresourceData;
	// Loads the texture and creates a default resource;
	hr = DirectX::LoadDDSTextureFromFile(device5, m_Path.c_str(), reinterpret_cast<ID3D12Resource**>(&m_pResource), m_DdsData, subresourceData);

	if (!D3D12GraphicsManager::SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to create texture: \'%s\'.\n", to_string(filePath).c_str());
		BL_SAFE_RELEASE(&m_pResource);
		return false;
	}

	std::wstring resourceName = m_Path + L"_DEFAULT_RESOURCE";
	graphicsManager->SucceededHRESULT(m_pResource->SetName(resourceName.c_str()));

	// Set resource desc created in LoadDDSTextureFromFile
	D3D12_RESOURCE_DESC resourceDesc = m_pResource->GetDesc();
	unsigned int rowPitch = subresourceData[0].RowPitch;

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

#pragma region CreateImmediateBuffer
	//D3D12_HEAP_PROPERTIES heapProps = {};
	//heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	//heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//heapProps.CreationNodeMask = 1; //used when multi-gpu
	//heapProps.VisibleNodeMask = 1; //used when multi-gpu
	//heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//
	//D3D12_RESOURCE_DESC uploadResourceDesc = {};
	//uploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//uploadResourceDesc.Width = m_Size;
	//uploadResourceDesc.Height = 1;
	//uploadResourceDesc.DepthOrArraySize = 1;
	//uploadResourceDesc.MipLevels = 1;
	//uploadResourceDesc.SampleDesc.Count = 1;
	//uploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//uploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	//
	//hr = graphicsManager->GetDevice()->CreateCommittedResource(
	//	&heapProps,
	//	D3D12_HEAP_FLAG_NONE,
	//	&uploadResourceDesc,
	//	D3D12_RESOURCE_STATE_COMMON,
	//	nullptr,
	//	IID_PPV_ARGS(&m_pResource)
	//);
	//
	//resourceName = filePath + L"_UPLOAD_RESOURCE";
	//
	//if (!graphicsManager->SucceededHRESULT(hr))
	//{
	//	BL_LOG_CRITICAL("Failed to create D3D12GraphicsTexture with name: \'%s\'\n", resourceName.c_str());
	//	return false;
	//}
	//
	//graphicsManager->SucceededHRESULT(m_pResource->SetName(resourceName.c_str()));
#pragma endregion

#pragma region CreateDescriptors
	// Create srv
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = resourceDesc.Format;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = m_NumMipLevels;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;

	m_ShaderResourceDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
	device5->CreateShaderResourceView(m_pResource, &desc, mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndex));
#pragma endregion

	CoInitialize(NULL);

	return true;
}

bool D3D12GraphicsTexture::CreateTexture2D(unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat, unsigned int textureUsage /* F_TEXTURE_USAGE */, const std::wstring name, D3D12_RESOURCE_STATES startStateTemp)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();
	DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();
	DescriptorHeap* rtvDHeap = graphicsManager->GetRTVDescriptorHeap();
	DescriptorHeap* dsvDHeap = graphicsManager->GetDSVDescriptorHeap();

	HRESULT hr;

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
	resourceDesc.MipLevels = 1;
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

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = resourceDesc.Format;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	hr = device5->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		startStateTemp,
		&clearValue,
		IID_PPV_ARGS(&m_pResource)
	);

	BL_ASSERT_MESSAGE(graphicsManager->SucceededHRESULT(hr), "Failed to create Resource with name: \'%s\'\n", name.c_str());


	hr = m_pResource->SetName(name.c_str());
	graphicsManager->SucceededHRESULT(hr);

#pragma endregion

#pragma region CreateDescriptors
	if (textureUsage & F_TEXTURE_USAGE::ShaderResource)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = dxgiFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		m_ShaderResourceDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndex);

		ID3D12Resource1* tmp = m_pResource;
		if (textureUsage & F_TEXTURE_USAGE::RayTracing)
		{
			tmp = nullptr;
		}

		device5->CreateShaderResourceView(tmp, &srvDesc, cdh);
	}

	if (textureUsage & F_TEXTURE_USAGE::RenderTarget)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = dxgiFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		m_RenderTargetDescriptorHeapIndex = rtvDHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = mainDHeap->GetCPUHeapAt(m_RenderTargetDescriptorHeapIndex);

		device5->CreateRenderTargetView(m_pResource, &rtvDesc, cdh);
	}

	if (textureUsage & F_TEXTURE_USAGE::UnorderedAccess)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = dxgiFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		uavDesc.Texture2D.PlaneSlice = 0;

		m_UnorderedAccessDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = mainDHeap->GetCPUHeapAt(m_UnorderedAccessDescriptorHeapIndex);

		device5->CreateUnorderedAccessView(m_pResource, nullptr, &uavDesc, cdh);
	}

	if (textureUsage & F_TEXTURE_USAGE::DepthStencil)
	{
		TODO("Fix format wrapper");
		// DXGI_FORMAT_R24_UNORM_X8_TYPELESS For the srv

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D.MipSlice = 0;

		m_DepthStencilDescriptorHeapIndex = dsvDHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = mainDHeap->GetCPUHeapAt(m_DepthStencilDescriptorHeapIndex);

		device5->CreateDepthStencilView(m_pResource, &dsvDesc, cdh);
	}

#pragma endregion

	return true;
}

unsigned int D3D12GraphicsTexture::GetShaderResourceHeapIndex() const
{
	BL_ASSERT(m_ShaderResourceDescriptorHeapIndex != -1);
	return m_ShaderResourceDescriptorHeapIndex;
}

unsigned int D3D12GraphicsTexture::GetRenderTargetHeapIndex() const
{
	BL_ASSERT(m_RenderTargetDescriptorHeapIndex != -1);
	return m_RenderTargetDescriptorHeapIndex;
}

unsigned int D3D12GraphicsTexture::GetUnorderedAccessIndex() const
{
	BL_ASSERT(m_UnorderedAccessDescriptorHeapIndex != -1);
	return m_UnorderedAccessDescriptorHeapIndex;
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
