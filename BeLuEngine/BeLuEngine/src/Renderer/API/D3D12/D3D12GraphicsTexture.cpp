#include "stdafx.h"
#include "D3D12GraphicsTexture.h"

TODO("This should be inside stdafx.h");
#include "../Misc/Log.h"

#include "D3D12GraphicsManager.h"

#include "../Renderer/DescriptorHeap.h"

D3D12GraphicsTexture::D3D12GraphicsTexture()
	:IGraphicsTexture()
{

}

D3D12GraphicsTexture::~D3D12GraphicsTexture()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
}
