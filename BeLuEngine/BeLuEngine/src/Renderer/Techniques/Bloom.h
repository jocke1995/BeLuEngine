#ifndef BLOOM_H
#define BLOOM_H

#include <array>

class IGraphicsTexture;

class Bloom
{
public:
	Bloom();
	virtual ~Bloom();

	unsigned int GetBlurWidth() const;
	unsigned int GetBlurHeight() const;

	IGraphicsTexture* GetPingPongTexture(unsigned int index) const;
	IGraphicsTexture* GetBrightTexture() const;

private:
	unsigned int m_BlurWidth = 1280;
	unsigned int m_BlurHeight = 720;

	// The compute shader will read and write in a "Ping-Pong"-order to these objects.
	std::array<IGraphicsTexture*, 2> m_PingPongTextures;

	// The final bright color will be stored in this texture
	IGraphicsTexture* m_BrightTexture;
};

#endif
