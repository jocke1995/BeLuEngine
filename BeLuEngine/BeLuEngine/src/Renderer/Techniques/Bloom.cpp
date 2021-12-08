#include "stdafx.h"
#include "Bloom.h"

#include "../Misc/Window.h"

#include "../API/IGraphicsTexture.h"

Bloom::Bloom()
{
	UINT resolutionWidth = 1280;
	UINT resolutionHeight = 720;

	m_BlurWidth = resolutionWidth;
	m_BlurHeight = resolutionHeight;

	// Create the bright texture
	m_BrightTexture = IGraphicsTexture::Create();
	m_BrightTexture->CreateTexture2D(
		resolutionWidth, resolutionHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::RenderTarget | F_TEXTURE_USAGE::ShaderResource,
		L"BrightTexture", D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_PingPongTextures[0] = IGraphicsTexture::Create();
	m_PingPongTextures[0]->CreateTexture2D(
		resolutionWidth, resolutionHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::RenderTarget | F_TEXTURE_USAGE::ShaderResource,
		L"PingPongTexture0", D3D12_RESOURCE_STATE_RENDER_TARGET);


	m_PingPongTextures[1] = IGraphicsTexture::Create();
	m_PingPongTextures[1]->CreateTexture2D(
		resolutionWidth, resolutionHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::RenderTarget | F_TEXTURE_USAGE::ShaderResource,
		L"PingPongTexture1", D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

Bloom::~Bloom()
{
	for (unsigned int i = 0; i < 2; i++)
	{
		BL_SAFE_DELETE(m_PingPongTextures[i]);
	}

	BL_SAFE_DELETE(m_BrightTexture);
}

unsigned int Bloom::GetBlurWidth() const
{
	return m_BlurWidth;
}

unsigned int Bloom::GetBlurHeight() const
{
	return m_BlurHeight;
}

IGraphicsTexture* Bloom::GetPingPongTexture(unsigned int index) const
{
	return m_PingPongTextures[index];
}

IGraphicsTexture* Bloom::GetBrightTexture() const
{
	return m_BrightTexture;
}
